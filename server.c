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

int server(int lport, int loop, int verbose) {
  unsigned int listen_port = lport;
  unsigned int server_port = 23456;
  struct sockaddr_in listen_addr, client_addr, server_addr;
  unsigned int addr_len, numbytes;
  int sockfd, sockfd2, numread, x ,rv, n;
  boolean donewithfile, send, wrongack;
  char filename[FILENAME];
  int numsends = 0;
  int trylimit = 6;
  int timeout = 4;
  int blocknum = 1;
  fd_set readfds;
  struct timeval tv;
  char errormessage[256] = "---RETRY LIMIT REACHED---";

  if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    printf("ERROR: socket\n");
    return EXIT_FAILURE;
  }

  listen_addr.sin_family = AF_INET;
  listen_addr.sin_port = htons((short)listen_port);
  listen_addr.sin_addr.s_addr = INADDR_ANY;
  memset(&(listen_addr.sin_zero), '\0', 8);

  if(bind(sockfd, (struct sockaddr *)&listen_addr, sizeof(struct sockaddr)) == -1) {
    perror("Bind Listen Port");
    return EXIT_FAILURE;
  }

  if((sockfd2 = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("Socket");
    return EXIT_FAILURE;
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons((short)server_port);
  server_addr.sin_addr.s_addr = INADDR_ANY;
  memset(&(server_addr.sin_zero), '\0', 8);

  if(bind(sockfd2, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
    perror("Bind Server Port");
    return EXIT_FAILURE;
  }

  do { //loop if -L flag
    /* RECEIVE RRQ */
    RRQStruct * rrq = (RRQStruct *)malloc(sizeof(RRQStruct));
    
    addr_len = sizeof(client_addr);
    if((numbytes = recvfrom(sockfd, rrq, sizeof(RRQStruct), 0, (struct sockaddr *)&client_addr, &addr_len)) == -1) {
      perror("recvfrom");
      return EXIT_FAILURE;
    }
    
    rrq->opcode = ntohs(rrq->opcode);
    
    memcpy(filename, rrq->filename, sizeof(filename));

    if(verbose) {  
      printf("\nRECEIVED---\n");
      printf("opcode:    %d\n", (int)rrq->opcode);
      printf("filename:  %s\n", rrq->filename);
      printf("\n");
    }

    free(rrq);
    
    /* SEND DATA PACKETS */
    FILE * file;
    if((file = fopen(filename, "rb")) == NULL) {
      perror("fopen");
      return EXIT_FAILURE;
    }
    
    do { //do until sent entire file
      DataStruct * data = (DataStruct *)malloc(sizeof(DataStruct));
      
      data->opcode = htons((short)DATAOP);
      data->blocknum = htons((short)blocknum);
      
      donewithfile = false;
      numread = (int)fread(data->data, sizeof(char), sizeof(data->data), file);
      if(numread < sizeof(data->data)) {
	donewithfile = true;
      }
  
      send = true;
      wrongack = false;
      numsends = 0;
      do { //resend data packets
	if(send && !wrongack) { //if (first send or resend) and (didn't just receive ack w/ wrong blocknum)
	  if((numbytes = sendto(sockfd2, data, numread + (2 * sizeof(short)), 0, (struct sockaddr *)&client_addr, addr_len)) == -1) {
	    perror("sendto");
	    return EXIT_FAILURE;
	  }
	  
	  if(verbose) {
	    printf("SENT---\n");
	    printf("numbytes: %d\n", numbytes);
	    printf("opcode:   %d\n", (int)ntohs(data->opcode));
	    printf("blocknum: %d\n", (int)ntohs(data->blocknum));
	    printf("data:\n");
	    for(x = 0; x < numread; x++) {
	      printf("%c", *(data->data + x));
	    }
	    printf("\n\n");
	  }
	}
	
	FD_ZERO(&readfds);
	FD_SET(sockfd2, &readfds);
	n = sockfd2 + 1;
	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	
	rv = select(n, &readfds, NULL, NULL, &tv);
	
	if(rv == -1) {
	  //error
	  perror("select");
	  return EXIT_FAILURE;
	} else if(rv == 0) { //timeout
	  wrongack = false;
	  numsends++;
	  if(verbose) {
	    printf("TIMEOUT... no ACK after %d seconds... send tries: #%d\n\n", timeout, numsends);
	  }
	  if(numsends == trylimit) {
            //send error packet to client
	    ErrorStruct * error = (ErrorStruct *)malloc(sizeof(ErrorStruct));

            error->opcode = htons((short)ERROP);
            error->errcode = htons((short)ERRCODE);
            memcpy(error->errmsg, errormessage, sizeof(error->errmsg));
            error->nullterm = 0;

            if((numbytes = sendto(sockfd2, error, (2 * sizeof(short)) + strlen(error->errmsg), 0, (struct sockaddr *)&client_addr, addr_len)) == -1) {
              perror("sendto");
              return EXIT_FAILURE;
            }

            if(verbose) {
              printf("SENT---\n");
              printf("numbytes: %d\n", numbytes);
              printf("opcode:   %d\n", (int)ntohs(error->opcode));
              printf("errcode:  %d\n", (int)ntohs(error->errcode));
              printf("errmsg:   %s\n", error->errmsg);
            }
            return EXIT_FAILURE;
          }
	} else { //recvfrom is ready
	  /* RECEIVE ACK */
	  AckStruct * ack = (AckStruct *)malloc(sizeof(AckStruct));
	  
	  if((numbytes = recvfrom(sockfd2, ack, sizeof(AckStruct), 0, (struct sockaddr *)&client_addr, &addr_len)) == -1) {
	    perror("recvfrom");
	    return EXIT_FAILURE;
	  }
	  
	  ack->opcode = ntohs(ack->opcode);
	  ack->blocknum = ntohs(ack->blocknum);
	  
	  if(ack->blocknum != blocknum) { //received wrong ack
	    wrongack = true;
	    if(verbose) {
	      printf("RECEIVED--- ack w/ incorrect blocknum\n");
	      printf("opcode:    %d\n", (int)ack->opcode);
	      printf("blocknum:  %d\n", (int)ack->blocknum);
	      printf("\n");
	    }
	  } else if(ack->blocknum == blocknum) { //received right ack
	    send = false;
	    if(verbose) {
	      printf("RECEIVED---\n");
	      printf("opcode:    %d\n", (int)ack->opcode);
	      printf("blocknum:  %d\n", (int)ack->blocknum);
	      printf("\n");
	    }
	    blocknum++;
	    free(data);
	    free(ack);
	  }  
	}
      } while(send && numsends <= trylimit);
    } while(!donewithfile);
    
    fclose(file);
    
  } while(loop);
  
  close(sockfd);
  close(sockfd2);
  
  return EXIT_SUCCESS;
}
