#ifndef DEBUG
#define DEBUG

#include "lang.h"

extern const char *printInstruction(Token *tok);
extern const char *printGraph(Node *tok);
extern const char *printTree(Node *tok);
extern void printNode(Node *node);
extern const char *retType(ReturnType ret);
extern const char *typeType(InstructionType type);

extern char *programLine(Program *prog, int num);
extern void printUnderline(Token *tok);
extern void printError(Program *prog, Node *node, const char *message);

#endif
