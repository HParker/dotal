#include "lang.h"
#include <stdio.h>
#include <stdlib.h>

const char *retType(ReturnType ret) {
  switch (ret) {
  case RET_I8:
    return "i8";
  case RET_I16:
    return "i16";
  case RET_I:
    return "i";
  case RET_I8S:
    return "i8s";
  case RET_I16S:
    return "i16s";
  case RET_CHAR:
    return "char";
  case RET_CHARS:
    return "chars";
  case RET_STRING:
    return "string";
  case RET_VOID:
    return "void";
  }
}

// TODO: this name really sucks
const char *typeType(InstructionType type) {
  switch (type) {
  case INSTR_BREAK:
    return "Break";
  case INSTR_I8:
    return "i8";
  case INSTR_I16:
    return "i16";
  case INSTR_I8S:
    return "i8s";
  case INSTR_I16S:
    return "i16s";
  case INSTR_CHAR:
    return "Char";
  case INSTR_CHARS:
    return "Chars";
  case INSTR_LITERAL_INT:
    return "INT";
  case INSTR_MEM_AREA:
    return "memA";
  case INSTR_MEM_LOCATION:
    return "memL";
  case INSTR_THEME:
    return "Theme";
  case INSTR_RETURN:
    return "Return";
  case INSTR_PAD:
    return "Pad";
  case INSTR_FUNCTION:
    return "fn";
  case INSTR_FUNCTION_CALL:
    return "fcall";
  case ISNTR_ARGUMENTS:
    return "Arg";
  case INSTR_LABEL:
    return "Label";
  case INSTR_PLUS:
    return "Plus";
  case INSTR_MINUS:
    return "Minus";
  case INSTR_TIMES:
    return "Times";
  case INSTR_DIVIDE:
    return "Divide";
  case INSTR_MODULO:
    return "Mod";
  case INSTR_GLOBAL_DEFINE:
    return "$Define";
  case INSTR_GLOBAL_ASSIGN:
    return "$Assign";
  case INSTR_GLOBAL_READ:
    return "$Read";
  case INSTR_ROOT:
    return "Node";
  case INSTR_LOCAL_DEFINE:
    return "Define";
  case INSTR_LOCAL_ASSIGN:
    return "Assign";
  case INSTR_LOCAL_ARRAY_ASSIGN:
    return "[]Assign";
  case INSTR_LOCAL_ARRAY_READ:
    return "[]Read";
  case INSTR_GLOBAL_ARRAY_ASSIGN:
    return "$[]Assign";
  case INSTR_GLOBAL_ARRAY_READ:
    return "$[]Read";
  case INSTR_LOCAL_READ:
    return "Read";
  case INSTR_PUT:
    return "Put";
  case INSTR_GET:
    return "Get";
  case INSTR_SEND:
    return "Send";
  case INSTR_PRINT:
    return "Print";
  case INSTR_CONDITION:
    return "Cond";
  case INSTR_WHILE:
    return "While";
  case INSTR_EQUALITY:
    return "EQ";
  case INSTR_NOT_EQUALITY:
    return "NOT EQ";
  case INSTR_AND:
    return "And";
  case INSTR_OR:
    return "Or";
  case INSTR_GREATER_THAN:
    return "Greater Than";
  case INSTR_LESS_THAN:
    return "Less Than";
  case INSTR_STRING_LITERAL:
    return "String";
  case INSTR_LITERAL_CHAR:
    return "Char";
  case INSTR_COLOR_BACKGROUND:
    return "Background";
  case INSTR_COLOR0:
    return "C0";
  case INSTR_COLOR1:
    return "C1";
  case INSTR_COLOR2:
    return "C2";
  case INSTR_SPRITE:
    return "Sprite";
  case INSTR_SPRITE_ROW:
    return "Sprite Row";
  }
}

void printNode(Node *node) {
  fprintf(stderr, "- <%s: \"%s\">\n", typeType(node->type), node->tok);
}

void print_helper(Node *n) {
  printf("  \"%i\" [label=\"%s: %s\" shape=\"box\"];\n", n->id, typeType(n->type), n->tok->str);

  for (int i = 0; i < n->children_index; i++) {
    print_helper(n->children[i]);
    printf("  \"%i\" -> \"%i\" [label=\"%s\"];\n", n->id, n->children[i]->id, retType(n->children[i]->ret));
  }
}

void printGraph(Node *s) {
  printf("digraph G {\n");

  print_helper(s);

  printf("}\n");
}

void printTree(Node *root) {
  printNode(root);
  for (int i = 0; i < root->children_index; i++) {
    printTree(root->children[i]);
  }
}

char *programLine(Program *prog, int num) {
  size_t len;
  char *line = NULL;
  FILE *file;
  file = fopen(prog->tok->filename, "r");
  if (!file) {
    fprintf(stderr, "could not open '%s'\n", prog->tok->filename);
    return "";
  } else {
    for (int i = 0; i < num; i++) {
      getline(&line, &len, file);
    }
    fclose(file);
  }
  return line;
}

void printUnderline(Token *start) {
  for (int i = 1; i < start->column - start->size; i++) {
    fprintf(stderr, " ");
  }

  fprintf(stderr, "└");
  for (int i = 1; i < start->size; i++) {
    fprintf(stderr, "─");
  }
  fprintf(stderr, "\n");
}

void printError(Program *prog, Node *node, const char *message) {
  prog->errored = 1;
  fprintf(stderr, "ERROR: %s -%s- :%s: - %s:%i:%i-%i\n", message,
          typeType(node->type), node->tok->str, node->tok->filename, node->tok->line,
          node->tok->column - node->tok->size, node->tok->column);
  fprintf(stderr, "%s", programLine(prog, node->tok->line));
  printUnderline(node->tok);
}
