#include "server.h"
#include <asm-generic/socket.h>
// #include <errno.h>
#include <netdb.h> // getnameinfo
#include <netinet/in.h>
#include <netinet/in.h> // sockaddr_in
#include <netinet/ip.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h> // socket APIs
#include <unistd.h>
#include <unistd.h> // open, close
// #include <time.h>

#define PORT "8080"

void signal_handler(int sig) {
  printf("SIG CAUGHT: %d\n", sig);
  exit(sig);
}

int main() {
  // Disable output buffering
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  printf("Hello, Easylink!\n");

  start_server(&PORT);

  return 0;
}
