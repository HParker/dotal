#include <stdio.h>
#include <stdlib.h>
#include "compiler.h"
#include "ast.h"

Node *progNode(int id, Token *tok, InstructionType type, ReturnType ret) {
  Node *n = malloc(sizeof(Node));
  n->type = type;
  n->ret = ret;
  n->children_index = 0;
  n->children_capacity = 20;

  n->memory_address = -1;

  n->id = id;
  n->tok = tok;

  n->children = malloc(sizeof(Node *) * n->children_capacity);
  return n;
}

Node *progEmptyNode(int id, InstructionType type, ReturnType ret) {
  Node *n = malloc(sizeof(Node));
  n->children_index = 0;
  n->children_capacity = 20;

  n->id = id;
  n->type = type;
  n->ret = ret;

  n->tok = malloc(sizeof(Token));
  n->tok->str = "Group";
  n->tok->line = 0;
  n->tok->column = 0;
  n->tok->size = 0;
  n->tok->filename = "";
  n->tok->lexCode = 0;

  n->memory_address = -1;

  n->children = malloc(sizeof(Node *) * n->children_capacity);
  return n;
}

void progAddChild(Node *n, Node *child) {
  if (n->children_capacity >= n->children_index + 1) {
    n->children_capacity += 10;
    n->children = realloc(n->children, sizeof(Node *) * n->children_capacity);
  }
  n->children[n->children_index++] = child;
}
