#include <asm-generic/socket.h>
#include <errno.h>
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
// #include <signal.h> // signal handling
// #include <time.h>

#define PORT "8080"

int main() {
  // Disable output buffering
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

  printf("Hello, Easylink!\n");

  int reuse_addr = 1, server_fd, client_addr_len, res_addr_info;
  struct addrinfo servinfo_base, *servinfo, *p; // points to the results
                                                // which is a linked-list with 1
                                                // or more addrinfo structs

  // a pointer to a struct `sockaddr_in` can be cast to a pointer to a
  // struct `sockaddr` and vice-versa. So even though connect() wants a
  // struct `sockaddr*`, you can still use a struct `sockaddr_in` and cast
  // it at the last minute
  struct sockaddr_in client_addr, serv_addr;

  // servinfo_base is basically the shopping list of what we _want_
  // getaddrinfo() to look for
  memset(&servinfo_base, 0, sizeof(servinfo_base)); // start with an empty list
  servinfo_base.ai_family = AF_INET;                // we want IPv4 or IPv6
  servinfo_base.ai_socktype = SOCK_STREAM; // we want TCP stream sockets
  servinfo_base.ai_flags = AI_PASSIVE;     // use my IP, aka bindable addresses

  //  we get is a linked list of addresses when calling getaddrinfo -> we save
  //  in servinfo. each node contains complete socket creation info it could be
  //  one, it could be many - who knows?
  res_addr_info = getaddrinfo(NULL, PORT, &servinfo_base, &servinfo);

  if (res_addr_info != 0) { // 0 = success, -1 = error
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(res_addr_info));
    return 1;
  }

  //
  // do all the socket stuff here
  //

  server_fd =
      socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
  if (server_fd == -1) {
    fprintf(stderr, "Socket creation failed: %s... \n",
            gai_strerror(server_fd));
    exit(1);
  }

  //
  // do serverstuff above this
  //
  freeaddrinfo(servinfo); // free the linked list

  int access_level =
      SOL_SOCKET; // access options on socket level, not protocol level
  int reuse_local_addresses = SO_REUSEADDR;
  if (setsockopt(server_fd, access_level, reuse_local_addresses, &reuse_addr,
                 sizeof(reuse_addr)) < 0) {
    printf("SO_REUSEADDR failed: %s \n", strerror(errno));
    return 1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(4221); // port number in Network Byte Order
                                    // (host to network short (2 byte number))
                                    //
  // TODO: Fix this
  // serv_addr.sin_addr = addr{htonl(INADDR_ANY)}; // IPv4 address

  if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
    printf("Bind failed: %s \n", strerror(errno));
    return 1;
  }

  int connection_backlog = 5;
  // marks a socket as accepting connections
  if (listen(server_fd, connection_backlog) != 0) {
    printf("Listen failed: %s \n", strerror(errno));
    return 1;
  }

  char hostBuffer[NI_MAXHOST], serviceBuffer[NI_MAXSERV];
  int error =
      getnameinfo((struct sockaddr *)&serv_addr, sizeof(serv_addr), hostBuffer,
                  sizeof(hostBuffer), serviceBuffer, sizeof(serviceBuffer), 0);

  if (error != 0) {
    printf("Error: %s\n", gai_strerror(error));
    return 1;
  }

  printf("\nServer is listening on http://%s:%s/\n\n", hostBuffer,
         serviceBuffer);

  client_addr_len = sizeof(client_addr);

  while (1) {
    // extracts first connection on queue of pending connection, creates a
    // socket and allocates a file descriptor
    int id =
        accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);

    char *buf = "HTTP/1.1 200 OK\r\n\r\n";

    send(id, (void *)buf, strlen(buf), 0);
  }
  close(server_fd);
  return 0;
}
