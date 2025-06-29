#include <asm-generic/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  // Disable output buffering
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

  printf("Hello, Easylink!\n");

  int server_fd, client_addr_len;
  struct sockaddr_in client_addr;

  int domain = AF_INET;   // Internet Protocol
  int type = SOCK_STREAM; // bidirectional, connection based - TCP
  int protocol = 0;       // Unspecified, let OS decide

  server_fd = socket(domain, type, protocol);
  if (server_fd == -1) {
    printf("Socket creation failed: %s... \n", strerror(errno));
    exit(1);
  }

  int reuse_addr = 1;
  int access_level =
      SOL_SOCKET; // access options on socket level, not protocol level
  int reuse_local_addresses = SO_REUSEADDR;
  if (setsockopt(server_fd, access_level, reuse_local_addresses, &reuse_addr,
                 sizeof(reuse_addr)) < 0) {
    printf("SO_REUSEADDR failed: %s \n", strerror(errno));
    return 1;
  }

  struct sockaddr_in serv_addr = {
      .sin_family = domain,
      .sin_port = htons(4221),         // port number
      .sin_addr = {htonl(INADDR_ANY)}, // IPv4 address
  };

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

  printf("Waiting for a client to connect...\n");
  client_addr_len = sizeof(client_addr);

  // extracts first connection on queue of pending connection, creates a socket
  // and allocates a file descriptor
  int id = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
  printf("Client connected\n");

  char *buf = "HTTP/1.1 200 OK\r\n\r\n";

  send(id, (void *)buf, strlen(buf), 0);

  close(server_fd);

  return 0;
}
