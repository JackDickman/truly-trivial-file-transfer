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

#include "ttftp.h"

#define USAGE_MESSAGE "usage: ttftp [-vL] [-h host -f filename] port"

int main(int argc, char * argv[]) {
  unsigned int listen_port;
  char * host = NULL;
  char * file = NULL;
  int loop = 1;
  int verbose = 0;

  int ch;
  while((ch = getopt(argc, argv, "vLh:f:")) != -1) {
    switch(ch) {
    case 'v':
      verbose = 1;
      break;
    case 'L':
      loop = 0;
      break;
    case 'h':
      host = strdup(optarg);
      break;
    case 'f':
      file = strdup(optarg);
      break;
    case '?':
    default:
      printf(USAGE_MESSAGE);
      return 0;
    }
  }

  argc -= optind;
  argv += optind;

  if(argc!= 1) {
    printf("%s\n", USAGE_MESSAGE);
    return EXIT_FAILURE;
  }

  listen_port = atoi(*argv);

  if(host && file) {
    client(host, listen_port, file, verbose);
  } else {
    server(listen_port, loop, verbose);
  }

  return EXIT_SUCCESS;
}
