#include "../includes/parser.h"
#include <asm-generic/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT "8080"
#define BUFFER_SIZE 256

// input for server create
// typedef struct {
//   const char *port;
//   int backlog;
//   int max_connections;
// } server_t_config;
//
// // output of server create
// typedef struct {
//   int server_fd;
//   struct addrinfo *servinfo; // linked list of addresses
//   server_t_config config;
//   int active;
// } server_t;
//
void signal_handler(int sig) {
  printf("SIG CAUGHT: %d\n", sig);
  exit(sig);
}

// server_t *create_server(const char *port) {
//   server_t *server = malloc(sizeof(server_t));
//   if (!server) {
//     perror("server_create: malloc");
//     return NULL;
//   }
//
//   server->server_fd = -1;
//   server->config.backlog = 5;
//   server->config.port = port;
//   server->config.max_connections = 1000;
//   server->servinfo = NULL;
//   server->active = 0;
//
//   return server;
// };

char *find_and_open_file(char path[]) {
  FILE *file_p;
  int file_size;
  size_t bytes_read;
  file_p = fopen(path, "r");

  if (file_p == NULL) {
    printf("File does not exist");
    exit(1);
  }

  fseek(file_p, 0, SEEK_END);
  file_size = ftell(file_p);
  fseek(file_p, 0, SEEK_SET);

  if (file_size == 0) {
    printf("File is empty!\n");
    fclose(file_p);
    exit(1);
  }

  if (file_size > BUFFER_SIZE) {
    perror("file: too large");
    fclose(file_p);
    exit(1);
  }

  char *data = malloc(file_size + 1);
  data[file_size] = '\0';

  bytes_read = fread(data, 1, file_size - 1, file_p);

  if (bytes_read == 0) {
    printf("No data read from file!\n");
    fclose(file_p);
    exit(1);
  }

  fclose(file_p);
  return data;
}

int main(int argc, char *argv[]) {
  const char *program_name = argv[0];
  const char *basename = strchr(program_name, '/');
  if (basename) {
    program_name = basename + 1;
  }

  if (argc < 2) {
    perror("usage: no file specified");
    return 1;
  }

  if (argc > 3) {
    perror("usage: too many arguments");
    return 1;
  }

  // Disable output dataing
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  printf("Hello, Easylink!\n");

  char *data = find_and_open_file(argv[1]);

  parser(data);

  free(data);

  return 0;
}
