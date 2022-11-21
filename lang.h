#ifndef LANG
#define LANG

#include <stdio.h>
#include "ast.h"
#include "lut.h"

typedef struct ExtraData {
  char *filename;
  int line;
  int col;
  int size;
} ExtraData;

typedef struct Instruction {
  InstructionType type;
  Token *tok;
} Instruction;

typedef struct StringNode {
  int capacity;
  int index;
  Node **nodes;
} StringNode;

typedef struct Program {
  int id;
  int errored;
  Node *root;

  Lut global_lut;
  Lut local_lut;
  Lut function_lut;

  StringNode strings;
  Token *tok;
  int used_system;
  int used_console;
  int used_screen;
  int used_mouse;
  int used_datetime;
  int used_print;
  int used_audio;
  int used_sine;

  void *scanner;
  void *parser;
  ExtraData data;

  FILE * output;
} Program;

#endif
