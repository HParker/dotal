#include "lang.h"
#include "parser.h"
#include <stdio.h>
// I don't like including the C file here,
// but it gives us access too yyTokenName[]
#include "compiler.h"
#include "parser.c"
#include "scanner.h"

// Black        0;30     Dark Gray     1;30
// Red          0;31     Light Red     1;31
// Green        0;32     Light Green   1;32
// Brown/Orange 0;33     Yellow        1;33
// Blue         0;34     Light Blue    1;34
// Purple       0;35     Light Purple  1;35
// Cyan         0;36     Light Cyan    1;36
// Light Gray   0;37     White         1;37

void printTokens(FILE *file) {
  char *no_color = "\033[0m";

  char *colors[] = {"\033[1;31m", "\033[1;32m", "\033[1;33m", "\033[1;34m",
                    "\033[1;35m", "\033[1;36m", "\033[1;33m", "\033[1;37m"};

  ExtraData datas[10000];
  int lexCodes[10000];
  int maxLexCode = 0;

  ExtraData data;
  data.line = 1;
  data.col = 1;
  data.size = 1;

  yyscan_t scanner;
  yylex_init_extra(&data, &scanner);

  yyset_in(file, scanner);

  int index = 0;
  int lexCode;
  do {
    lexCode = yylex(scanner);
    lexCodes[index] = lexCode;
    datas[index].line = data.line;
    datas[index].col = data.col;
    datas[index].size = data.size;
    index++;

    if (lexCode > maxLexCode) {
      maxLexCode = lexCode;
    }
  } while (lexCode > 0);

  rewind(file);

  int charIndex = 1;
  int lineIndex = 1;
  int dataIndex = 0;

  int c;
  while ((c = fgetc(file)) != EOF) {
    if (dataIndex > 0 && (datas[dataIndex - 1].col) == charIndex &&
        datas[dataIndex - 1].line == lineIndex) {
      printf("%s", no_color);
    }

    if ((datas[dataIndex].col - datas[dataIndex].size) == charIndex &&
        datas[dataIndex].line == lineIndex) {
      printf("%s", colors[lexCodes[dataIndex] % 8]);
      dataIndex++;
    }

    printf("%c", c);
    charIndex++;

    if (c == '\n') {
      lineIndex++;
      charIndex = 1;
    }
  }

  printf("\n");
  for (int i = 0; i <= maxLexCode; i++) {
    printf("%s█ - %s%s\n", colors[i % 8], yyTokenName[i], no_color);
  }
}

int main(int argc, char **argv) {
  FILE *file;
  if (argc > 1) {
    file = fopen(argv[1], "r");
    if (!file) {
      fprintf(stderr, "could not open %s\n", argv[1]);
      return 1;
    }
  } else {
    fprintf(stderr, "Please provide a file\n");
    return 1;
  }

  printTokens(file);
}
