#include "server.h"
#include <asm-generic/socket.h>
// #include <errno.h>
#include <netdb.h> // getnameinfo
#include <netinet/in.h>
#include <netinet/in.h> // sockaddr_in
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h> // socket APIs
#include <unistd.h>
#include <unistd.h> // open, close
// #include <time.h>

#define BACKLOG 5

void start_server(const char *port) {
  int reuse_addr = 1, server_fd, socket_opts, binding, conn, res_addr_info,
      yes = 1;
  struct addrinfo servinfo_base, *servinfo, *p;

  struct sockaddr_in client_addr, serv_addr;

  memset(&servinfo_base, 0, sizeof(servinfo_base)); // start with an empty list
  servinfo_base.ai_family = AF_INET;                // we want IPv4 or IPv6
  servinfo_base.ai_socktype = SOCK_STREAM; // we want TCP stream sockets
  servinfo_base.ai_flags = AI_PASSIVE;     // use my IP, aka bindable addresses

  res_addr_info = getaddrinfo(NULL, port, &servinfo_base, &servinfo);

  if (res_addr_info != 0) { // 0 = success, -1 = error
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(res_addr_info));
    exit(1);
  }

  for (p = servinfo; p != NULL; p = p->ai_next) {
    server_fd = socket(servinfo->ai_family, servinfo->ai_socktype,
                       servinfo->ai_protocol);
    if (server_fd == -1) {
      perror("server: socket");
      continue;
    }

    socket_opts =
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if (socket_opts == -1) {
      perror("setsockopt");
      exit(1);
    }

    binding = bind(server_fd, p->ai_addr, p->ai_addrlen);
    if (binding == -1) {
      close(server_fd);
      perror("server: bind");
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "server: failed to bind\n");
    exit(1);
  }

  // marks a socket as accepting connections
  if (listen(server_fd, BACKLOG) != 0) {
    perror("server: listen");
    exit(1);
  }

  char hostBuffer[NI_MAXHOST], serviceBuffer[NI_MAXSERV];
  int name_info =
      getnameinfo(p->ai_addr, p->ai_addrlen, hostBuffer, sizeof(hostBuffer),
                  serviceBuffer, sizeof(serviceBuffer), 0);

  if (name_info != 0) {
    perror("server: name_info");
    exit(1);
  }

  freeaddrinfo(servinfo); // free the linked list, dont need it anymore

  printf("\nServer is listening on http://%s:%s/\n\n", hostBuffer,
         serviceBuffer);

  while (1) {
    // extracts first connection on queue of pending connection, creates a
    // socket and allocates a *new* file descriptor
    //
    // we now have 2 open socket file descriptors,
    // one that listens for new connections and
    // another that handles an accepted connection
    socklen_t sin_size = sizeof client_addr;
    int connection_fd =
        accept(server_fd, (struct sockaddr *)&client_addr, &sin_size);

    // we will do the http parsing stuff here
    char *buf = "HTTP/1.1 200 OK\r\n\r\n";
    int len, bytes_sent;

    // send returns the amount of bytes that were sent, if it is less than the
    // length of the buffer we need to send the rest so will probably need logic
    // for that
    bytes_sent = send(connection_fd, (void *)buf, strlen(buf), 0);
    if (bytes_sent == -1) {
      perror("server: send");
    }

    close(connection_fd);
  }
  close(server_fd);
}
