#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include "parse.h"

int nextToken(char * buf, FILE * file) {
  int c = 0;
  unsigned char ch;
  const char null = '\0', space = ' ', newline = '\n', tab = '\t', semicolon = ';';
  buf[0] = null;
  while ((ch = fgetc(file)) && !feof(file)) {
    if (ch == semicolon)
      while (ch != newline) ch = fgetc(file);
    if (ch == newline || ch == space || ch == tab) {
      if (c == 0) continue;
      buf[c++] = null;
      break;
    }
    buf[c++] = ch;
  }
  return c;
}

int nextIntToken(char * token, FILE * file, int * result) {
  if (nextToken(token, file) == 0) return -1;
  *result = strtol(token, &token, 10);
  return 1;
}

int nextStringToken(char * token, FILE * file, char * result) {
  if (nextToken(token, file) == 0) return -1;
  return strncmp(token, result, sizeof(*result)) == 0 ? 0 : -1;
}

int nextFloatToken(char * token, FILE * file, float * result) {
  if (nextToken(token, file) == 0) return -1;
  *result = strtod(token, &token);
  return 1;
}
