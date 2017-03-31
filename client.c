#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <assert.h>
#include <sys/select.h>

#include "ttftp.h"

int client(char * host, int lport, char * file, int verbose) {
  unsigned int client_port = 12345;
  unsigned int listen_port = lport;
  struct sockaddr_in client_addr, listen_addr, server_addr;
  unsigned int numbytes, addr_len;
  int sockfd, x, n, rv;
  struct hostent * he;
  char * to_host = host;
  boolean stop = false;
  char * filename = file;
  fd_set readfds;
  struct timeval tv;
  int lingertime = 3;

  client_addr.sin_family = AF_INET;
  client_addr.sin_port = htons((short)client_port);
  client_addr.sin_addr.s_addr = INADDR_ANY;
  memset(&(client_addr.sin_zero), '\0', 8);  

  listen_addr.sin_family = AF_INET;
  listen_addr.sin_port = htons((short)listen_port);
  listen_addr.sin_addr.s_addr = INADDR_ANY;
  memset(&(listen_addr.sin_zero), '\0', 8);

  if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("socket");
    return EXIT_FAILURE;
  }

  if(bind(sockfd, (struct sockaddr *)&client_addr, sizeof(struct sockaddr)) == -1) {
    perror("bind");
    return EXIT_FAILURE;
  }

  if((he = gethostbyname(to_host)) == NULL) {
    perror("gethostbyname");
    return EXIT_FAILURE;
  }

  listen_addr.sin_addr = *((struct in_addr *)he->h_addr);                                                                      
  memset(&(listen_addr.sin_zero), '\0', 8);

  /* SEND RRQ */
  RRQStruct * rrq = (RRQStruct *)malloc(sizeof(RRQStruct));
  rrq->opcode = htons((short)RRQOP);
  memcpy(rrq->filename, filename, sizeof(rrq->filename));
  rrq->nullterm = 0;

  if((numbytes = sendto(sockfd, rrq, sizeof(RRQStruct), 0, (struct sockaddr *)&listen_addr, sizeof(struct sockaddr))) == -1) {
    perror("sendto");
    return EXIT_FAILURE;
  }

  if(verbose) {
    printf("\nSENT---\n");
    printf("opcode:    %d\n", (int)ntohs(rrq->opcode));
    printf("filename:  %s\n", rrq->filename);
    printf("\n");
  }

  free(rrq);

  do {
    /* RECEIVE DATA OR ERROR */
    DataStruct * buffer = (DataStruct *)malloc(sizeof(DataStruct));

    addr_len = sizeof(server_addr);  
    if((numbytes = recvfrom(sockfd, buffer, sizeof(DataStruct), 0, (struct sockaddr *)&server_addr, &addr_len)) == -1) {
      perror("recvfrom");
      return EXIT_FAILURE;
    }
    
    buffer->opcode = ntohs(buffer->opcode);

    DataStruct * databuf = (DataStruct *)malloc(sizeof(DataStruct));
    ErrorStruct * errorbuf = (ErrorStruct *)malloc(sizeof(ErrorStruct));
    if(buffer->opcode == DATAOP) {
      //received a data packet
      memcpy(databuf, buffer, sizeof(DataStruct));
      databuf->blocknum = ntohs(databuf->blocknum);
    } else if(buffer->opcode == ERROP) {
      //received an error packet
      memcpy(errorbuf, buffer, sizeof(ErrorStruct));
      errorbuf->errcode = ntohs(errorbuf->errcode);
      if(verbose) {
	printf("RECEIVED---\n");
	printf("numbytes: %d\n", numbytes);
	printf("opcode:   %d\n", (int)errorbuf->opcode);
	printf("errcode:  %d\n", (int)errorbuf->errcode);
      }
      printf("errmsg:   %s\n", errorbuf->errmsg);
      return EXIT_FAILURE;
    }

    if(numbytes < sizeof(DataStruct)) {
      stop = true;
    }

    if(verbose) {
      printf("RECEIVED---\n");
      printf("numbytes:  %d\n", numbytes);
      printf("opcode:    %d\n", (int)databuf->opcode);
      printf("blocknum:  %d\n", (int)databuf->blocknum);
      printf("data:\n");
      for(x = 0; x < numbytes - 4; x++) {
	printf("%c", *(databuf->data + x));
      }
      printf("\n\n");
    } else {
      for(x = 0; x < numbytes - 4; x++) {
        printf("%c", *(databuf->data + x));
      }
    }
    
    /* SEND ACK */
    AckStruct * ack = (AckStruct *)malloc(sizeof(AckStruct));
    
    ack->opcode = htons((short)databuf->opcode);
    ack->blocknum = htons((short)databuf->blocknum);
    //ack->blocknum = htons((short)9); //for testing ack w/ wrong blocknum on first ack
    //ack->blocknum = htons((short)1); //for testing ack w/wrong blocknum on second ack
    
    if((numbytes = sendto(sockfd, ack, sizeof(AckStruct), 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr))) == -1) {
      perror("sendto");
      return EXIT_FAILURE;
    }
    
    if(verbose) {
      printf("SENT---\n");
      printf("opcode:   %d\n", (int)ntohs(ack->opcode));
      printf("blocknum: %d\n", (int)ntohs(ack->blocknum));
      printf("\n");
    }

    /* LINGER */
    if(stop) { //linger if already received last data packet
      DataStruct * lingerdata = (DataStruct *)malloc(sizeof(DataStruct));
      
      FD_ZERO(&readfds);
      FD_SET(sockfd, &readfds);
      n = sockfd + 1;
      tv.tv_sec = lingertime;
      tv.tv_usec = 0;
      
      rv = select(n, &readfds, NULL, NULL, &tv);
    
      if(rv == -1) { //error
	perror("select");
	return EXIT_FAILURE;
      } else if(rv == 0) { //timeout
	if(verbose) {
	  printf("---LINGERED %d SECONDS... PROTOCAL COMPLETE---\n", lingertime);
	}
      } else { //recvfrom is ready
	if((numbytes = recvfrom(sockfd, lingerdata, sizeof(DataStruct), 0, (struct sockaddr *)&server_addr, &addr_len)) == -1) {
	  perror("recvfrom");
	  return EXIT_FAILURE;
	}
	  
	lingerdata->opcode = ntohs(lingerdata->opcode);
	lingerdata->blocknum = ntohs(lingerdata->blocknum);
      
	if(verbose) {
	  printf("RECEIVED---\n");
	  printf("numbytes:  %d\n", numbytes);
	  printf("opcode:    %d\n", (int)lingerdata->opcode);
	  printf("blocknum:  %d\n", (int)lingerdata->blocknum);
	  printf("data:\n");
	  for(x = 0; x < numbytes - 4; x++) {
	    printf("%c", *(lingerdata->data + x));
	  }
	  printf("\n\n");
	} else {
	  for(x = 0; x < numbytes - 4; x++) {
	    printf("%c", *(lingerdata->data + x));
	  }
	}
      }
      free(lingerdata);
    }

    free(buffer);
    free(databuf);
    free(errorbuf);
    free(ack);
  } while(!stop);

  close(sockfd);

  return EXIT_SUCCESS;
}
