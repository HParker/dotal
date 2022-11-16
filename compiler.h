#ifndef COMPILER
#define COMPILER

#include <stdio.h>
#include "lang.h"

extern void setupCompiler(Program * prog);
extern void resetCompiler(Program * prog);
extern void fromString(Program * prog, char * str);
extern void fromStdin(Program * prog);
extern void fromFile(Program * prog, char * file);
extern void toFile(Program * prog, char * file);
extern void parse(Program * prog);

extern ReturnType InstrToRet(InstructionType instr);
extern ReturnType memoryIdentifierType(Program *prog, Node *node, char *loc1, char *loc2);
extern void verifyTypes(Program * prog, Node * node, Node * lhs, Node * rhs);
extern Node *progImplicitPad(Program * prog, Node * node);

#endif
