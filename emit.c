#include "compiler.h"
#include "lut.h"
#include "debug.h"
#include "lang.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EMIT(str, ...) fprintf(prog->output, str, ##__VA_ARGS__)

void emit_condition(Program *prog, Node *node) {
  if (node->children_index > 2) {
    EMIT("  ;default-%i JCN2\n", node->id);
    EMIT("  ;else-%i JMP2\n", node->id);
  } else if (node->children_index == 2) {
    EMIT("  ;default-%i JCN2\n", node->id);
    EMIT("  ;after-%i JMP2\n", node->id);
  } else {
    printError(prog, node, "condition has no true or false paths");
  }
}

int add_string(Program *prog, Node *node) {
  if (prog->strings.index == prog->strings.capacity) {
    prog->strings.capacity += 10;
    prog->strings.nodes = realloc(prog->strings.nodes, prog->strings.capacity);
  }

  for (int i = 0; i < prog->strings.index; i++) {
    if (strcmp(prog->strings.nodes[i]->tok->str, node->tok->str) == 0) {
      return i;
    }
  }
  prog->strings.nodes[prog->strings.index++] = node;
  return prog->strings.index - 1;
}

void emit_self(Program *prog, Node *node) {
  /* fprintf(stderr, "EMITTING %s\n", tokType(node->tok->str)); */
  switch (node->type) {
  case INSTR_BREAK:
    EMIT("    BREAKPOINT\n");
    break;
  case INSTR_CONDITION:
    emit_condition(prog, node);
    break;
  case INSTR_WHILE:
    EMIT("  ;while-block-%i JCN2\n", node->id);
    EMIT("  ;while-end-%i JMP2\n", node->id);
    break;
  case INSTR_PAD:
    EMIT("    #00 SWP ( i8 -> i16 )\n");
    break;
  case INSTR_FUNCTION: {
    int id = lutFindFn(&prog->function_lut, node);
    if (id == -1) {
      printError(prog, node, "Function not found - in definition! how does this happen!");
    } else {
      EMIT("@%s-%i\n", prog->function_lut.names[id], id);
    }
    break;
  }
  case INSTR_LABEL: {
    // This has no args or returns so, we look it up by name only
    int id = lutFind(&prog->function_lut, node);
    if (id == -1) {
      // TODO: it might be good to register implicit labels,
      //       but for now we can just emit them
      EMIT("  ;%s\n", node->tok->str);
    } else {
      EMIT("  ;%s-%i\n", prog->function_lut.names[id], id);
    }
    break;
  }
  case INSTR_GLOBAL_ASSIGN:
    switch (prog->global_lut.types[lutFind(&prog->global_lut, node)]) {
      case RET_I8:
      EMIT("    ;global-heap #%04x ADD2 STA ( assign global %s )\n", node->loc, node->tok->str);
      break;
    case RET_I16:
      EMIT("    ;global-heap #%04x ADD2 STA2 ( assign global %s )\n", node->loc, node->tok->str);
      break;
    default:
      printError(prog, node, "local assign only supports i8 and i16 implement it!");
      break;
    }
    break;
  case INSTR_GLOBAL_DEFINE:
    // skip
    break;
  case INSTR_GLOBAL_READ:
    if (node->ret == RET_I8) {
      EMIT("    ;global-heap #%04x ADD2 LDA ( load global %s )\n", node->loc, node->tok->str);
    } else if (node->ret == RET_I16) {
      EMIT("    ;global-heap #%04x ADD2 LDA2 ( load global %s )\n", node->loc, node->tok->str);
    }
    break;
  case INSTR_DIVIDE:
    if (node->children_index > 2) {
      printError(prog, node, "plus has wrong number of children");
    }

    if (node->ret == RET_I8) {
      EMIT("    DIV\n");
    } else if (node->ret == RET_I16) {
      EMIT("    DIV2\n");
    }

    break;
  case INSTR_FUNCTION_CALL: {
    int id = lutFindFnCall(&prog->function_lut, node);
    if (id == -1) {
      printError(prog, node, "Function not found");
    } else {
      EMIT("  ;%s-%i JSR2\n", prog->function_lut.names[id], id);

      for (int i = 0; i < prog->function_lut.arg_count[id] && i < MAX_STACK_SLOTS; i++) {
        if (prog->function_lut.args[id][i] == RET_I16) {
          EMIT("POP2 ( removing i16 stack arg )\n");
        } else {
          EMIT("POP ( removing  i8 stack arg  )\n");
        }
      }
    }
    break;
  }

  case ISNTR_ARGUMENTS:
    // Ignore arguments.
    // TODO: we might not actually need to keep these in the tree?
    break;
  case INSTR_LOCAL_READ: {
    if (node->loc < 0) {
      if (node->loc == -1) {
        if (node->ret == RET_I8 || node->ret == RET_CHAR) {
          EMIT("    DUP ( read local from stack i8 %s )\n", node->tok->str);
        } else if (node->ret == RET_I16) {
          EMIT("    DUP2 ( read local from stack %s )\n", node->tok->str);
        } else {
          fprintf(stderr, "************************* error failed to emit local read\n");
        }
      } else if (node->loc == -2) {
        if (node->ret == RET_I8 || node->ret == RET_CHAR) {
          EMIT("    OVR ( read local from stack i8 %s )\n", node->tok->str);
        } else if (node->ret == RET_I16) {
          EMIT("    OVR2 ( read local from stack %s )\n", node->tok->str);
        } else {
          fprintf(stderr, "************************* error failed to emit local read\n");
        }
      }
      break;
    }

    if (node->ret == RET_I8 || node->ret == RET_CHAR) {
      EMIT("    ;local-heap #%04x ADD2 LDA ( read local %s )\n", node->loc, node->tok->str);
    } else if (node->ret == RET_I16) {
      EMIT("    ;local-heap #%04x ADD2 LDA2 ( read local %s )\n", node->loc, node->tok->str);
    } else {
      fprintf(stderr, "************************* error failed to emit local read\n");
    }
    break;
  }
  case INSTR_LOCAL_ASSIGN: {
    if (node->loc < 0) {
      // Skip when we saved the local on the stack only
      break;
    }
    switch (node->ret) {
    case RET_CHAR:
    case RET_I8:
      EMIT("    ;local-heap #%04x ADD2 STA ( assign local i8 %s )\n", node->loc, node->tok->str);
      break;
    case RET_I16:
      EMIT("    ;local-heap #%04x ADD2 STA2 ( assign local %s )\n", node->loc, node->tok->str);
      break;
    default:
      printError(prog, node, "********* local assign only supports i8 and i16 implement it!");
      fprintf(stderr, "XXXXXXXXXX %s\n", retType(node->ret));
      break;
    }

    /* if (node->children[0]->ret == RET_I8) { */
    /*   EMIT("    ;local-heap #%04x ADD2 STA ( assign local %s )\n", node->loc, node->tok->str); */
    /* } else if (node->children[0]->ret == RET_I16) { */
    /*   EMIT("    ;local-heap #%04x ADD2 STA2 ( assign local %s )\n", node->loc,node->tok->str); */
    /* } else { */
    /*   // TODO: add support for char and string types */
    /*   printError(prog, node, "local assign only supports i8 and i16 implement it!"); */
    /* } */
    break;
  }
  case INSTR_LOCAL_ARRAY_ASSIGN: {
    if (node->children[0]->ret == RET_I8 || node->children[0]->ret == RET_CHAR) {
      // The second ADD2 is the array index
      EMIT("    ;local-heap #%04x ADD2 ADD2 STA ( assign local %s )\n", node->loc, node->tok->str);
    } else if (node->children[0]->ret == RET_I16) {
      // TODO
      EMIT("    #0002 MUL2 ;local-heap #%04x ADD2 ADD2 STA2 ( assign local %s )\n", node->loc, node->tok->str);
    } else {
      printError(prog, node, "local array assign only supports i8 and i16 implement it!");
      fprintf(stderr, "type: %i\n", node->children[0]->ret);
    }
    break;
  }
  case INSTR_LOCAL_ARRAY_READ: {
    if (node->ret == RET_I8 || node->ret == RET_CHAR) {
      // The second ADD2 is the array index
      EMIT("    ;local-heap #%04x ADD2 ADD2 LDA ( read local %s )\n", node->loc, node->tok->str);
    } else if (node->ret == RET_I16) {
      EMIT("    #0002 MUL2 ;local-heap #%04x ADD2 ADD2 LDA2 ( assign local %s )\n", node->loc, node->tok->str);
    } else {
      printError(prog, node, "local array read only supports i8 and i16 implement it!");
      fprintf(stderr, "type: %i\n", node->ret);
    }
    break;
  }
  case INSTR_LITERAL_INT:
    if (node->ret == RET_I8) {
      EMIT("    LIT %02x ( literal %s )\n", atoi(node->tok->str), node->tok->str);
    } else if (node->ret == RET_I16) {
      EMIT("    LIT2 %04x ( literal %s )\n", atoi(node->tok->str), node->tok->str);
    } else {
      EMIT("    LIT2 %04x ( literal %s )\n", atoi(node->tok->str), node->tok->str);
    }
    break;
  case INSTR_LITERAL_CHAR:
    EMIT("    LIT '%c ( character literal %s )\n", node->tok->str[0], node->tok->str);
    break;
  case INSTR_GLOBAL_ARRAY_ASSIGN: {
    if (node->children[0]->ret == RET_I8) {
      // The second ADD2 is the array index
      EMIT(
          "    ;global-heap #%04x ADD2 ADD2 STA ( global array assign %s )\n", node->loc, node->tok->str);
    } else if (node->children[0]->ret == RET_I16) {
      // TODO
      EMIT("    #0002 MUL2 ;global-heap #%04x ADD2 ADD2 STA2 ( global array assign %s )\n", node->loc, node->tok->str);
    } else {
      printError(prog, node, "global array assign only supports i8 and i16 implement it!");
    }
    break;
  }
  case INSTR_GLOBAL_ARRAY_READ: {
    if (node->ret == RET_I8) {
      // The second ADD2 is the array index
      EMIT("    ;global-heap #%04x ADD2 ADD2 LDA ( read array global %s )\n", node->loc, node->tok->str);
    } else if (node->ret == RET_I16) {
      EMIT("    #0002 MUL2 ;global-heap #%04x ADD2 ADD2 LDA2 ( read array global %s )\n", node->loc, node->tok->str);
    } else {
      printError(prog, node, "global array read only supports i8 and i16 implement it!");
    }
    break;
  }
  case INSTR_LOCAL_DEFINE: {
    /* localLutInsert(prog, node, node->children[0]->ret); */
    break;
  }
  case INSTR_CHAR:
    EMIT("    LIT %02x ( literal %s )\n", node->tok->str[0], node->tok->str);
    break;
  case INSTR_MINUS:
    if (node->children_index > 2) {
      printError(prog, node, "plus has wrong number of children");
    }

    if (node->children[0]->ret == RET_I8) {
      EMIT("    SUB\n");
    } else if (node->children[0]->ret == RET_I16) {
      EMIT("    SUB2\n");
    } else {
      printError(prog, node, "what type is this?");
    }
    break;
  case INSTR_PLUS:
    if (node->children_index > 2) {
      printError(prog, node, "plus has wrong number of children");
    }

    if (node->children[0]->ret == RET_I8) {
      EMIT("    ADD\n");
    } else if (node->children[0]->ret == RET_I16) {
      EMIT("    ADD2\n");
    } else {
      printError(prog, node, "what type is this?");
    }
    break;
  case INSTR_EQUALITY:
    if (node->children_index > 2) {
      printError(prog, node, "equality has wrong number of children");
    }

    if (node->children[0]->ret == RET_I8) {
      EMIT("    EQU\n");
    } else if (node->children[0]->ret == RET_I16) {
      EMIT("    EQU2\n");
    }
    break;
  case INSTR_NOT_EQUALITY:
    if (node->children_index > 2) {
      printError(prog, node, "not equals has wrong number of children");
    }

    if (node->children[0]->ret == RET_I8) {
      EMIT("    NEQ\n");
    } else if (node->children[0]->ret == RET_I16) {
      EMIT("    NEQ2\n");
    }
    break;
  case INSTR_GREATER_THAN:
    if (node->children[0]->ret == RET_I8) {
      EMIT("    GTH\n");
    } else if (node->children[0]->ret == RET_I16) {
      EMIT("    GTH2\n");
    }
    break;
  case INSTR_LESS_THAN:
    if (node->children[0]->ret == RET_I8) {
      EMIT("    LTH\n");
    } else if (node->children[0]->ret == RET_I16) {
      EMIT("    LTH2\n");
    }
    break;
  case INSTR_MODULO:
    if (node->children[0]->ret == RET_I8) {
      EMIT("    MOD ( mod )\n");
    } else if (node->children[0]->ret == RET_I16) {
      EMIT("    MOD2 ( mod )\n");
    }
    break;
  case INSTR_AND:
    if (node->children[0]->ret == RET_I8) {
      EMIT("    AND\n");
    } else if (node->children[0]->ret == RET_I16) {
      EMIT("    AND2\n");
    }
    break;
  case INSTR_OR:
    if (node->children[0]->ret == RET_I8) {
      EMIT("    ORA\n");
    } else if (node->children[0]->ret == RET_I16) {
      EMIT("    ORA2\n");
    }
    break;
  case INSTR_I16:
    /* EMIT(" $2\n"); */
    break;
  case INSTR_I8:
    /* EMIT(" $1\n"); */
    break;
  case INSTR_I16S:
    /* EMIT(" $2\n"); */
    break;
  case INSTR_I8S:
    /* EMIT(" $1\n"); */
    break;
  case INSTR_MEM_AREA:
    EMIT("    .%s/%s\n", node->tok->str, node->children[0]->tok->str);
    break;
  case INSTR_MEM_LOCATION:
    // skip
    break;
  case INSTR_STRING_LITERAL:
    EMIT("    ;str-%i ( String address )\n", add_string(prog, node));
    break;
  case INSTR_PUT: {
    switch (node->children[0]->ret) {
    case RET_STRING:
      EMIT("    ;print-str JSR2 ( Print string )\n");
      break;
    case RET_I8:
      EMIT("    PRINT_I8 ( Print number )\n");
      break;
    case RET_I16:
      EMIT("    PRINT_I16 ( Print number )\n");
      break;
    case RET_I:
      // TODO: ret is not yet defined...
      // maybe I can do this better in another way...
      // Might resolve itself if we move this out of
      EMIT("    PRINT_I8 ( Print number )\n");
      break;
    case RET_CHAR:
      EMIT("    EMIT ( Print Char )\n");
      break;
    case RET_VOID:
      fprintf(stderr, "ERROR: can't print void\n");
      break;
    default:
      fprintf(stderr, "WHAt arE YOU PRINtING????????? %i\n",
              node->children[0]->ret);
      // TODO: this is used for arrays temp...
      EMIT("    PRINT_I8 ( Print number )\n");
    }
    EMIT("    #0a EMIT ( Finish with a newline )\n");
    break;
  }

  case INSTR_GET:
    if (node->ret == RET_I8) {
      EMIT("    DEI\n");
    } else {
      EMIT("    DEI2\n");
    }
    break;
  case INSTR_SEND:
    // First child is the locationsv
    if (node->children[1]->ret == RET_I8) {
      EMIT("    DEO\n");
    } else {
      EMIT("    DEO2\n");
    }
    break;
  case INSTR_PRINT: {
    switch (node->children[0]->ret) {
    case RET_STRING:
      EMIT("    ;print-str JSR2 ( Print string )\n");
      break;
    case RET_I8:
      EMIT("    PRINT_I8 ( Print number )\n");
      break;
    case RET_I16:
      EMIT("    PRINT_I16 ( Print number )\n");
      break;
    case RET_I8S:
      EMIT("    PRINT_I16 ( WARNING no idea how to print arrays yet )\n");
      break;
    case RET_I16S:
      EMIT("    PRINT_I16 ( WARNING CANNOT PRINT ARRAYS YET )\n");
      break;
    case RET_CHAR:
      EMIT("    EMIT ( Print Char )\n");
      break;
    case RET_VOID:
      printError(prog, node, "can't print void");
      break;
    case RET_I:
      printError(prog, node, "can't print unspecified type");
      break;
    }
    break;
  }
  case INSTR_RETURN:
    EMIT("  JMP2r\n");
    break;
  case INSTR_ROOT:
    // SKIP
    break;
  case INSTR_THEME:
    EMIT("  #%c%c%c%c .System/r DEO2\n", node->children[0]->tok->str[1],
           node->children[1]->tok->str[1], node->children[2]->tok->str[1],
           node->children[3]->tok->str[1]);
    EMIT("  #%c%c%c%c .System/g DEO2\n", node->children[0]->tok->str[2],
           node->children[1]->tok->str[2], node->children[2]->tok->str[2],
           node->children[3]->tok->str[2]);
    EMIT("  #%c%c%c%c .System/b DEO2\n", node->children[0]->tok->str[3],
           node->children[1]->tok->str[3], node->children[2]->tok->str[3],
           node->children[3]->tok->str[3]);
    break;
  case INSTR_TIMES:
    if (node->children_index > 2) {
      printError(prog, node, "plus has wrong number of children");
    }

    if (node->ret == RET_I8) {
      EMIT("    MUL\n");
    } else if (node->ret == RET_I16) {
      EMIT("    MUL2\n");
    }
    break;
  case INSTR_COLOR_BACKGROUND:
    break;
  case INSTR_COLOR0:
    break;
  case INSTR_COLOR1:
    break;
  case INSTR_COLOR2:
    break;
  case INSTR_SPRITE:
    EMIT("@sprite_%s", node->tok->str);
    break;
  case INSTR_SPRITE_ROW: {
    EMIT(" ");
    int x;
    x = 0;
    for (int i = 0; i < 4; i++) {
      if (node->tok->str[3 - i] == '1') {
        x |= (1 << i);
      }
    }
    EMIT("%x", x);
    x = 0;
    for (int i = 0; i < 4; i++) {
      if (node->tok->str[7 - i] == '1') {
        x |= (1 << i);
      }
    }
    EMIT("%x", x);
    break;
  }
  }
}

void build_helper(Program *prog, Node *node) {
  /* fprintf(stderr, "doing node: %s %i\n", node->tok->str->str, node->type); */
  if (node->type == INSTR_LOCAL_DEFINE) {
    emit_self(prog, node);
    return;
  }

  if (node->type == INSTR_CONDITION) {
    if (node->children_index > 2) {
      if (node->children[0]->ret != RET_I8) {
        printError(prog, node->children[0], "conditions must return i8 to prevent eventual stack overflow");
      }

      build_helper(prog, node->children[0]); // CONDITION

      emit_self(prog, node); // CONDITIONAL JUMPS

      EMIT("  @default-%i ( true case )\n", node->id);
      build_helper(prog, node->children[1]); // TRUE PATH

      EMIT("  ;after-%i JMP2 ( true case done )\n", node->id);
      EMIT("  @else-%i ( else )\n", node->id);
      build_helper(prog, node->children[2]); // ELSE PATH
      EMIT("  @after-%i ( done )\n", node->id);
    } else if (node->children_index == 2) {
      if (node->children[0]->ret != RET_I8) {
        printError(prog, node->children[0], "conditions must return i8 to prevent eventual stack overflow");
      }

      build_helper(prog, node->children[0]); // CONDITION

      emit_self(prog, node); // CONDITIONAL JUMPS

      EMIT("  @default-%i ( true case )\n", node->id);

      build_helper(prog, node->children[1]); // TRUE PATH

      EMIT("  @after-%i ( else )\n", node->id);

    } else {
      printError(prog, node,
                 "conditional had a hard to understand number of conditions");
    }
    return;
  }

  if (node->type == INSTR_GLOBAL_DEFINE) {
    emit_self(prog, node);
    return;
  }

  if (node->type == INSTR_SPRITE) {
    emit_self(prog, node);
    for (int i = 0; i < node->children_index; i++) {
      build_helper(prog, node->children[i]);
    }
    EMIT("\n");
    return;
  }

  if (node->type == INSTR_WHILE) {
    EMIT("  @while-cond-%i ( while condition )\n", node->id);
    build_helper(prog, node->children[0]); // CONDITION

    emit_self(prog, node); // CHECK...

    EMIT("  @while-block-%i ( while block  )\n", node->id);

    build_helper(prog, node->children[1]); // work
    EMIT("  ;while-cond-%i JMP2 ( Skip to condition case )\n", node->id);
    EMIT("  @while-end-%i ( end while )\n", node->id);
    return;
  }

  if (node->type == INSTR_FUNCTION) {
    emit_self(prog, node);

    // arguments need to pull off the stack in reverse order
    for (int i = node->children[0]->children_index - 1; i >= 0; i--) {
      build_helper(prog, node->children[0]->children[i]);
    }

    int returned = 0;
    for (int i = 1; i < node->children_index; i++) {
      if (node->children[i]->type == INSTR_RETURN) {
        returned = 1;
      }
      build_helper(prog, node->children[i]);
    }

    if (returned == 0) {
      EMIT("BRK ( end of function )\n"); // TODO: skip BRK if we always return
    }
    return;
  }

  for (int i = 0; i < node->children_index; i++) {
    build_helper(prog, node->children[i]);
  }

  emit_self(prog, node);
}

void build_instruction_list(Program *prog, Node *node) {
  // prelude
  EMIT("%%BREAKPOINT { #0101 #0e DEO2 }\n");
  EMIT("%%EMIT { #18 DEO }\n");
  EMIT("%%EMIT2 { #18 DEO2 }\n");
  EMIT("%%PRINT_I8 {\n");
  EMIT("  DUP #64 DIV DUP #30 ADD EMIT #64 MUL SUB\n");
  EMIT("  DUP #0a DIV DUP #30 ADD EMIT #0a MUL SUB\n");
  EMIT("  #30 ADD EMIT\n");
  EMIT("}\n");

  EMIT("%%PRINT_I16 {\n");
  EMIT("  DUP2 #03e8 DIV2 DUP2 #0030 ADD2 EMIT2 #03e8 MUL2 SUB2\n");
  EMIT("  DUP2 #0064 DIV2 DUP2 #0030 ADD2 EMIT2 #0064 MUL2 SUB2\n");
  EMIT("  DUP2 #000a DIV2 DUP2 #0030 ADD2 EMIT2 #000a MUL2 SUB2\n");
  EMIT("  LIT2 0030 ADD2 EMIT2\n");
  EMIT("}\n");
  EMIT("%%MOD  { DUP2 DIV MUL SUB }\n");
  EMIT("%%MOD2 { DIV2k MUL2 SUB2 }\n");
  EMIT("\n");
  if (prog->used_system)
    EMIT("|00 @System [ &vector $2 &pad $6   &r $2 &g $2 &b $2 ]\n");
  if (prog->used_console)
    EMIT("|10 @Console [ &vector $2 &pad $6 &write $1 ]\n");
  if (prog->used_screen)
    EMIT("|20 @Screen [ &vector $2 &width $2 &height $2 &pad   $2 &x $2 &y $2 &addr $2 &pixel $1 &sprite $1 ]\n");
  if (prog->used_audio) {
    EMIT("|30 @Audio0 [ &vector $2 &position $2 &output $1 &pad $3 &adsr $2 &length $2 &addr $2 &volume $1 &pitch $1 ]\n");
    EMIT("|40 @Audio1 [ &vector $2 &position $2 &output $1 &pad $3 &adsr $2 &length $2 &addr $2 &volume $1 &pitch $1 ]\n");
    EMIT("|50 @Audio2 [ &vector $2 &position $2 &output $1 &pad $3 &adsr $2 &length $2 &addr $2 &volume $1 &pitch $1 ]\n");
    EMIT("|60 @Audio3 [ &vector $2 &position $2 &output $1 &pad $3 &adsr $2 &length $2 &addr $2 &volume $1 &pitch $1 ]\n");
    // All 4 chanels
  }


  if (prog->used_mouse)
    EMIT("|90 @Mouse [ &vector $2 &x $2 &y $2 &state $1 &wheel $1 ]\n");
  /* EMIT("|a0 @File   [ &vector $2 &success $2 &offset-hs $2 &offset-ls $2 &name $2 &length $2 &load $2 &save $2 ]\n"); */
  if (prog->used_datetime)
    EMIT("|b0 @DateTime   [ &year   $2 &month    $1 &day    $1 &hour  $1 &minute $1 &second $1 &dotw $1 &doty $2 &isdst $1 ]\n");

  int mainFound = 0;
  for (int i = 0; i < prog->function_lut.index; i++) {
    if (strcmp(prog->function_lut.names[i], "main") == 0 &&
        prog->function_lut.arg_count[i] == 0) {
      mainFound = 1;
      EMIT("|0100 ;%s-%i JSR2\nBRK\n", prog->function_lut.names[i], i);
    }
  }

  if (mainFound == 0) {
    printError(prog, prog->root, "No main method found");
    exit(1);
  }

  build_helper(prog, prog->root);

  if (prog->strings.index > 0 && prog->used_print) {
    EMIT("( Strings )\n");
    EMIT("@print-str\n");
    EMIT("  &loop\n");
    EMIT("    LDAk #18 DEO\n");
    EMIT("    INC2 LDAk ,&loop JCN\n");
    EMIT("  POP2\n");
    EMIT("JMP2r\n");
    for (int i = 0; i < prog->strings.index; i++) {
      EMIT("@str-%i \"", i);
      int strIndex = 0;
      while (prog->strings.nodes[i]->tok->str[strIndex] != '\0') {
        if (prog->strings.nodes[i]->tok->str[strIndex] == ' ') {
          EMIT(" 20 \"");
        } else {
          EMIT("%c", prog->strings.nodes[i]->tok->str[strIndex]);
        }
        strIndex++;
      }
      EMIT(" $1\n");
    }
  }

  if (prog->global_lut.max > 0) {
    EMIT("( global heap )\n");
    EMIT("@global-heap $%i\n", prog->global_lut.max);
  }

  if (prog->local_lut.all_time_max > 0) {
    EMIT("( local heap )\n");
    EMIT("@local-heap $%i\n", prog->local_lut.all_time_max);
  }

  if (prog->used_sine) {
    EMIT("@sine\n");
    EMIT("  8083 8689 8c8f 9295 989b 9ea1 a4a7 aaad\n");
    EMIT("  b0b3 b6b9 bbbe c1c3 c6c9 cbce d0d2 d5d7\n");
    EMIT("  d9db dee0 e2e4 e6e7 e9eb ecee f0f1 f2f4\n");
    EMIT("  f5f6 f7f8 f9fa fbfb fcfd fdfe fefe fefe\n");
    EMIT("  fffe fefe fefe fdfd fcfb fbfa f9f8 f7f6\n");
    EMIT("  f5f4 f2f1 f0ee eceb e9e7 e6e4 e2e0 dedb\n");
    EMIT("  d9d7 d5d2 d0ce cbc9 c6c3 c1be bbb9 b6b3\n");
    EMIT("  b0ad aaa7 a4a1 9e9b 9895 928f 8c89 8683\n");
    EMIT("  807d 7a77 7471 6e6b 6865 625f 5c59 5653\n");
    EMIT("  504d 4a47 4542 3f3d 3a37 3532 302e 2b29\n");
    EMIT("  2725 2220 1e1c 1a19 1715 1412 100f 0e0c\n");
    EMIT("  0b0a 0908 0706 0505 0403 0302 0202 0202\n");
    EMIT("  0102 0202 0202 0303 0405 0506 0708 090a\n");
    EMIT("  0b0c 0e0f 1012 1415 1719 1a1c 1e20 2225\n");
    EMIT("  2729 2b2e 3032 3537 3a3d 3f42 4547 4a4d\n");
    EMIT("  5053 5659 5c5f 6265 686b 6e71 7477 7a7d\n");
  }
}
