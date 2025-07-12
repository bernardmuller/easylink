#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define BREAK '\r'
#define LINE_BREAK '\n'

void parser(char *data) {
  size_t index = 2;
  char chunk[] = "";

  while (index < strlen(data)) {
    printf("%c\n", data[index]);
    index = index + 2;
  }
}
