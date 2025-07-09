#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define REDIS_PONG "+PONG\r\n"
#define DUMMY_COMMAND "*2\r\n$4\r\nECHO\r\n$3\r\nhey\r\n"

void *handle_client_thread(void *arg_fd) {
  int client_fd = *(int *)arg_fd;
  free(arg_fd);

  char buffer[128];
  char *pong = REDIS_PONG;
  while (1) {
    ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
      if (bytes_received == 0) {
        printf("Client disconnected\n");
      } else {
        perror("recv");
      }
      break;
    }
    // Null-terminate the received data
    buffer[bytes_received] = '\0';
    ssize_t bytes_sent = send(client_fd, pong, strlen(pong), 0);
    if (bytes_sent == -1) {
      perror("client_thread: send");
      break;
    }
  }
  return NULL;
}

int main() {
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

  int server_fd;
  struct sockaddr_in client_addr;

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    printf("Socket creation failed: %s...\n", strerror(errno));
    return 1;
  }

  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) <
      0) {
    printf("SO_REUSEADDR failed: %s \n", strerror(errno));
    return 1;
  }

  struct sockaddr_in serv_addr = {
      .sin_family = AF_INET,
      .sin_port = htons(6379),
      .sin_addr = {htonl(INADDR_ANY)},
  };

  if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
    printf("Bind failed: %s \n", strerror(errno));
    return 1;
  }

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    printf("Listen failed: %s \n", strerror(errno));
    return 1;
  }

  printf("Waiting for clients to connect...\n");

  while (1) {
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd =
        accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);

    if (client_fd == -1) {
      perror("accept");
      continue;
    }

    int *client_fd_p = malloc(sizeof(int));
    if (client_fd_p == NULL) {
      perror("malloc: client file descriptor pointer");
      close(client_fd);
      continue;
    }
    *client_fd_p = client_fd;

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, handle_client_thread, client_fd_p) !=
        0) {
      perror("pthread_create");
      free(client_fd_p);
      close(client_fd);
      continue;
    }

    pthread_detach(thread_id);
  }

  close(server_fd);
  return 0;
}
