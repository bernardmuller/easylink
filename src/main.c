#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT "8080"
#define BUFFER_SIZE 256
#define USAGE_MSG "./program_name <path/to/file>"
#define MAX_TOKENS 256

typedef struct Error {
  enum ErrorType {
    ERROR_NONE = 0,
    ERROR_TODO,
    ERROR_MEMORY,
    ERROR_ARGUMENTS,
    ERROR_UNEXPECTED_CHAR,
    ERROR_USAGE,
    ERROR_MALFORMED
  } type;
  const char *reference;
  const char *message;
} Error;

typedef enum TokenType {
  TOKEN_STRING,
  TOKEN_ERROR_START,
  TOKEN_NUMBER,
  TOKEN_BULK_STRING,
  TOKEN_ARRAY,
  TOKEN_LINE_BREAK,
  TOKEN_CARRIAGE_RETURN,
  TOKEN_CRLF,
  TOKEN_EOF
} TokenType;

typedef struct Token {
  TokenType type;
  char *character;
  size_t position;
} Token;

typedef struct {
  Token *data;
  int size;
} TokenArray;

Error ok = {ERROR_NONE, NULL, NULL};

#define NEW_ERROR(n, t, ref, msg)                                              \
  Error n;                                                                     \
  (n).type = (t);                                                              \
  (n).reference = (ref);                                                       \
  (n).message = (msg);

static char *get_error_type_string(enum ErrorType type) {
  switch (type) {
  case ERROR_ARGUMENTS:
    return "Bad Arguments";
  case ERROR_USAGE:
    return "Usage";
  case ERROR_TODO:
    return "Not implemented yet";
  case ERROR_UNEXPECTED_CHAR:
    return "Unexpected character";
  default:
    return "No error :)";
  }
}

char *stringdup(const char *str) {
  size_t len = strlen(str) + 1;
  char *copy = malloc(len);
  if (copy) {
    memcpy(copy, str, len);
  }
  return copy;
}

char *get_error_string(Error err) {
  static char err_str[128];
  char *div = ": ";
  char *type_err = get_error_type_string(err.type);

  snprintf(err_str, sizeof(err_str), "%s%s%s", err.reference, div, type_err);
  return err_str;
}

void print_error(Error err) {
  char *err_str = get_error_string(err);
  printf("%s: %s\n", err_str, err.message);
}

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

char peek(char *data, size_t position) {
  if (position >= strlen(data)) {
    return '\0';
  }
  return data[position + 1];
}

Token get_next_token(char *data, size_t *current_pos) {
  Error err;
  size_t pos = *current_pos;
  while (data[pos] != '\0') {
    char current_char = data[pos];
    if (current_char == '*') {
      pos++;
      *current_pos = pos;
      return (Token){TOKEN_ARRAY, "*", pos};
    }
    if (current_char == '$') {
      pos++;
      *current_pos = pos;
      return (Token){TOKEN_BULK_STRING, "$", pos};
    }
    if (current_char == '+') {
      pos++;
      *current_pos = pos;
      return (Token){TOKEN_STRING, "+", pos};
    }
    if (current_char == '-') {
      pos++;
      *current_pos = pos;
      return (Token){TOKEN_ERROR_START, "-", pos};
    }
    if (current_char == '\\') {
      if (peek(data, pos) == 'n') {
        pos = pos + 2;
        *current_pos = pos;
        return (Token){TOKEN_LINE_BREAK, "\\n", pos};
      }
      if (peek(data, pos) == 'r') {
        if (peek(data, pos + 1) != '\\' && peek(data, pos + 2) != 'n') {
          NEW_ERROR(err, ERROR_MALFORMED, "get_next_token: CRLF",
                    "malformed carriage return line feed")
          print_error(err);
          exit(1);
        }
        pos = pos + 4;
        *current_pos = pos;
        return (Token){TOKEN_CRLF, "\\r\\n", pos};
      }
    }
    if (isdigit(data[pos]) != 0) {
      int buf_idx = 0;
      static char buf[10];
      while (data[pos] != '\\' && peek(data, pos) != '\r') {
        buf[buf_idx++] = data[pos];
        pos++;
      }
      buf[buf_idx] = '\0';
      *current_pos = pos;
      return (Token){TOKEN_NUMBER, stringdup(buf), pos};
    }
    if (isprint(current_char) != 0) {
      static char buf[1024];
      int buf_idx = 0;
      while (data[pos] != '\\' && peek(data, pos) != 'r') {
        buf[buf_idx++] = data[pos];
        pos++;
      }
      buf[buf_idx] = '\0';
      *current_pos = pos;
      return (Token){TOKEN_STRING, stringdup(buf), pos};
    }

    NEW_ERROR(err, ERROR_UNEXPECTED_CHAR, "get_next_token",
              "unexpected character")
    print_error(err);
    exit(1);
  }
  return (Token){TOKEN_EOF, "\\0", pos};
}

TokenArray tokenize(char *data) {
  Error err;
  if (!data) {
    NEW_ERROR(err, ERROR_ARGUMENTS, "tokenize", "no data provided to parse");
    print_error(err);
    exit(1);
  }

  size_t position = 0;
  int token_count = 0;
  Token *tokens = malloc(sizeof(Token) * MAX_TOKENS);
  TokenArray token_data;

  while (position < strlen(data)) {
    Token token = get_next_token(data, &position);
    tokens[token_count++] = token;
    if (token.type == TOKEN_EOF) {
      break;
    }
  }

  token_data.data = tokens;
  token_data.size = token_count;
  return token_data;
}

void free_tokens(TokenArray *tokens) {
  for (int i = 0; i < tokens->size; i++) {
    if (tokens->data[i].type == TOKEN_STRING) {
      free(tokens->data[i].character);
    }
  }
  free(tokens->data);
}

void handle_args(int argc, char **argv) {
  Error err;
  const char *program_name = argv[0];
  const char *basename = strchr(program_name, '/');
  if (basename) {
    program_name = basename + 1;
  }

  if (argc < 2) {
    NEW_ERROR(err, ERROR_USAGE, "main", USAGE_MSG);
    print_error(err);
    exit(1);
  }

  if (argc > 3) {
    NEW_ERROR(err, ERROR_USAGE, "main", USAGE_MSG);
    print_error(err);
    exit(1);
  }
}

void print_tokens(TokenArray tokens) {
  for (int i = 0; i < tokens.size; i++) {
    printf("Token %d: type=%d, character='%s', position=%zu\n", i,
           tokens.data[i].type, tokens.data[i].character,
           tokens.data[i].position);
  }
}

int main(int argc, char **argv) {
  handle_args(argc, argv);

  printf("Easylink!\n\n");

  char *data = find_and_open_file(argv[1]);

  TokenArray tokens = tokenize(data);

  print_tokens(tokens);

  free(data);
  free_tokens(&tokens);
  exit(0);
}
