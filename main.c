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
#define BACKLOG 5 // only hold a maximum of 5 pending connections

int main() {
  // Disable output buffering
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

  printf("Hello, Easylink!\n");

  // for http servers the order of operations are:
  // getraddrinfo();
  // socket();
  // bind();
  // listen();

  int reuse_addr = 1, server_fd, socket_opts, binding, conn, res_addr_info,
      yes = 1;
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

  for (p = servinfo; p != NULL; p->ai_next) {
    server_fd = socket(servinfo->ai_family, servinfo->ai_socktype,
                       servinfo->ai_protocol);
    if (server_fd == -1) {
      perror("server: socket");
      continue;
    }

    // reuse addresses
    socket_opts = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes,
                             sizeof(reuse_addr));
    if (socket_opts == -1) {
      perror("setsockopt");
      exit(1);
    }

    // bind the addrress to a name, any socket address is initially unnamed
    // because we are writing a server and not a client, we are using bind()
    // instead of connect()
    binding = bind(server_fd, p->ai_addr, p->ai_addrlen);
    if (binding == -1) {
      close(server_fd);
      perror("server: bind");
      continue;
    }

    break;
  }

  freeaddrinfo(servinfo); // free the linked list

  if (p == NULL) {
    fprintf(stderr, "server: failed to bind\n");
    exit(1);
  }

  // marks a socket as accepting connections
  if (listen(server_fd, BACKLOG) != 0) {
    perror("server: listen");
    exit(1);
  }

  // get the host address information so we can print a cool message
  // stating where we are listening
  char hostBuffer[NI_MAXHOST], serviceBuffer[NI_MAXSERV];
  int name_info =
      getnameinfo((struct sockaddr *)&serv_addr, sizeof(serv_addr), hostBuffer,
                  sizeof(hostBuffer), serviceBuffer, sizeof(serviceBuffer), 0);

  if (name_info != 0) {
    perror("server: name_info");
    exit(1);
  }

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
  return 0;
}
