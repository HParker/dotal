%include {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lang.h"
#include "compiler.h"
#include "lut.h"
#include "debug.h"
}

%token_type                { Token* }
%type function             { Node* }
%type functions            { Node* }
%type type                 { Node* }
%type function_body        { Node* }
%type conditional_elses    { Node* }
%type conditional_else     { Node* }
%type argument             { Node* }
%type arguments            { Node* }
%type argument_definition  { Node* }
%type argument_definitions { Node* }
%type statements           { Node* }
%type statement            { Node* }
%type fcall                { Node* }
%type return_type          { Node* }
%type expression           { Node* }
%type operator             { Node* }
%type location             { Node* }
%extra_argument { Program *prog }

%left EQUALS EXPR_AND EXPR_OR.
%left EQUALITY NOT_EQUALITY GREATER_THAN LESS_THAN.
%left PLUS MINUS.
%left TIMES SLASH.
%left MODULO.

%syntax_error
{
  fprintf(stderr, "Syntax error: Unexpected token of lexcode %i - %s - %s:%i:%i-%i\n%s",
          prog->tok->lexCode,
          prog->tok->str,
          prog->tok->filename,
          prog->tok->line,
          prog->tok->column,
          prog->tok->column + prog->tok->size,
          programLine(prog, prog->tok->line));
  printUnderline(prog->tok);
  prog->errored = 1;
}

program ::= functions(funcs). {
  progAddChild(prog->root, funcs);
  /* prog->root = funcs; */
}

functions(node) ::= functions(funcs) function(func). {
  node = funcs;
  progAddChild(funcs, func);
}

functions(node) ::= . {
  node = progEmptyNode(prog->id++, INSTR_ROOT, RET_VOID);
}

function(node) ::= SPRITE IDENTIFIER(sprite_name) INT_LITERAL(row1) INT_LITERAL(row2) INT_LITERAL(row3) INT_LITERAL(row4) INT_LITERAL(row5) INT_LITERAL(row6) INT_LITERAL(row7) INT_LITERAL(row8). {
  node = progNode(prog->id++, sprite_name, INSTR_SPRITE, RET_VOID);
  progAddChild(node, progNode(prog->id++, row1, INSTR_SPRITE_ROW, RET_VOID));
  progAddChild(node, progNode(prog->id++, row2, INSTR_SPRITE_ROW, RET_VOID));
  progAddChild(node, progNode(prog->id++, row3, INSTR_SPRITE_ROW, RET_VOID));
  progAddChild(node, progNode(prog->id++, row4, INSTR_SPRITE_ROW, RET_VOID));
  progAddChild(node, progNode(prog->id++, row5, INSTR_SPRITE_ROW, RET_VOID));
  progAddChild(node, progNode(prog->id++, row6, INSTR_SPRITE_ROW, RET_VOID));
  progAddChild(node, progNode(prog->id++, row7, INSTR_SPRITE_ROW, RET_VOID));
  progAddChild(node, progNode(prog->id++, row8, INSTR_SPRITE_ROW, RET_VOID));
}

// Type definition
// Global
function(node) ::= GLOBAL IDENTIFIER(IDENT) DEFINE type(t). {
  node = progNode(prog->id++, IDENT, INSTR_GLOBAL_DEFINE, RET_VOID);
  // TODO: is this child required?
  // if I get rid of it, I need to clean up the nodes
  progAddChild(node, t);

  lutInsert(&prog->global_lut, node, InstrToRet(t->type));
}

// local
statement(node) ::= IDENTIFIER(IDENT) DEFINE type(t). {
  node = progNode(prog->id++, IDENT, INSTR_LOCAL_DEFINE, RET_VOID);
  // TODO: is the child needed here?
  // if I get rid of it, I need to clean up the nodes
  // Alternatively type can just stay a token?
  // I'll have to think
  progAddChild(node, t);
  lutInsert(&prog->local_lut, node, InstrToRet(t->type));
}

type(node) ::= I16(t). {
  node = progNode(prog->id++, t, INSTR_I16, RET_VOID);
}

type(node) ::= I8(t). {
  node = progNode(prog->id++, t, INSTR_I8, RET_VOID);
}

type(node) ::= CHAR(t). {
  node = progNode(prog->id++, t, INSTR_CHAR, RET_VOID);
}

type(node) ::= CHARS(t). {
  node = progNode(prog->id++, t, INSTR_CHARS, RET_VOID);
}

type(node) ::= INT_LITERAL(index) I8S(t). {
  node = progNode(prog->id++, t, INSTR_I8S, RET_VOID);
  progAddChild(node, progNode(prog->id++, index, INSTR_LITERAL_INT, RET_I16));
}

type(node) ::= INT_LITERAL(index) CHARS(t). {
  node = progNode(prog->id++, t, INSTR_CHARS, RET_VOID);
  progAddChild(node, progNode(prog->id++, index, INSTR_LITERAL_INT, RET_I16));
}

type(node) ::= INT_LITERAL(index) I16S(t). {
  node = progNode(prog->id++, t, INSTR_I16S, RET_VOID);
  progAddChild(node, progNode(prog->id++, index, INSTR_LITERAL_INT, RET_I16));
}

// Function
function(node) ::= FN IDENTIFIER(fn_name) OPEN_PAREN argument_definitions(arg) CLOSE_PAREN  return_type(RT) DO_BLOCK statements(body) END_BLOCK. {
  node = progNode(prog->id++, fn_name, INSTR_FUNCTION, InstrToRet(RT->type));
  progAddChild(node, arg);
  progAddChild(node, body);

  for (int i = 0; i < body->children_index; i++) {
    if (body->children[i]->ret != RET_VOID && body->children[i]->ret != InstrToRet(RT->type)) {
      // TODO: re-enable this after fixing LOCAL READ and assign use of RET
      /* printError(prog, body->children[i], "function returns a type that is not the return type of the function"); */
    }
  }

  // TODO: this should be handled in the LUT for us.
  prog->function_lut.args[prog->function_lut.index] = malloc(sizeof(ReturnType) * arg->children_index);
  prog->function_lut.arg_count[prog->function_lut.index] = arg->children_index;
  for (int i = 0; i < arg->children_index; i++) {
    prog->function_lut.args[prog->function_lut.index][i] = InstrToRet(arg->children[i]->children[0]->type);
  }

  lutInsertFn(&prog->function_lut, node, InstrToRet(RT->type));

  // Update search offset so that local functions are scoped to the function
  if (prog->local_lut.max > prog->local_lut.all_time_max) {
    prog->local_lut.all_time_max = prog->local_lut.max;
  }
  prog->local_lut.search_offset = prog->local_lut.index;
}


argument_definitions(node) ::= . {
  node = progEmptyNode(prog->id++, INSTR_ROOT, RET_VOID);
}

argument_definitions(node) ::= argument_definition(arg). {
  node = progEmptyNode(prog->id++, INSTR_ROOT, RET_VOID);
  progAddChild(node, arg);
}

argument_definitions(node) ::= argument_definitions(nodes) COMMA argument_definition(arg). {
  progAddChild(nodes, arg);
  node = nodes;
}

argument_definition(node) ::= IDENTIFIER(IDENT) COLON type(t). {
  node = progNode(prog->id++, IDENT, INSTR_LOCAL_ASSIGN, InstrToRet(t->type));

  // TODO: The type node is not really needed
  // but if we get rid of it we need to clean it up.
  progAddChild(node, t);

  int ivarId = lutInsert(&prog->local_lut, node, InstrToRet(t->type));
  if (ivarId == -1) {
    // TODO: this should be impossible... we just inserted it right?
    printError(prog, node, "cannot assign to argument? why: this is a compiler bug");
  } else {
    node->loc = prog->local_lut.locs[ivarId];
  }
}

return_type(node) ::= COLON type(return_type). {
  node = return_type;
}

return_type(node) ::= . {
  node = progEmptyNode(prog->id++, INSTR_ROOT, RET_VOID);
}


fcall(node) ::= IDENTIFIER(func) OPEN_PAREN arguments(args_node) CLOSE_PAREN. {
  node = progNode(prog->id++, func, INSTR_FUNCTION_CALL, RET_VOID);
  progAddChild(node, args_node);

  int functionId = lutFindFnCall(&prog->function_lut, node);

  if (functionId == -1) {
    deferLookup(&prog->function_lut, node);
  } else {
    if (args_node->children_index != prog->function_lut.arg_count[functionId]) {
      printError(prog, node, "wrong number of arguments");
      fprintf(stderr, "%i != %i\n", args_node->children_index, prog->function_lut.arg_count[functionId]);
    } else {
      if (prog->function_lut.types[functionId] != RET_VOID) {
        printError(prog, node, "calling function with a return value that isn't used. This is now allowed");
      } else {
        node->ret = prog->function_lut.types[functionId];
      }
    }
  }
}

statements(node) ::= statements(states) statement(state). {
  if (state->type == INSTR_RETURN) {
    states->ret = state->ret;
  }
  progAddChild(states, state);
  node = states;
}

statements(node) ::= . {
  node = progEmptyNode(prog->id++, INSTR_ROOT, RET_VOID);
}

statement(node) ::= BREAK(b). {
  node = progNode(prog->id++, b, INSTR_BREAK, RET_VOID);
}

statement(node) ::= PUT(p) OPEN_PAREN expression(expr) CLOSE_PAREN. {
  // TODO: this function is on the axe
  prog->used_print = 1;

  node = progNode(prog->id++, p, INSTR_PUT, RET_VOID);
  progAddChild(node, expr);
}

statement(node) ::= SEND(p) OPEN_PAREN location(loc) COMMA expression(expr) CLOSE_PAREN. {
  // TODO: ret is wrong here
  node = progNode(prog->id++, p, INSTR_SEND, RET_VOID);

  if (expr->ret == RET_I) {
    expr->ret = loc->ret;
  }

  if (strcmp(expr->tok->str, "sine") == 0) {
    prog->used_sine = 1;
  }
  progAddChild(node, expr);
  progAddChild(node, loc);
}

expression(node) ::= COLON IDENTIFIER(label). {
  node = progNode(prog->id++, label, INSTR_LABEL, RET_I16);
}


expression(node) ::= GET(p) OPEN_PAREN location(loc) CLOSE_PAREN. {
  node = progNode(prog->id++, p, INSTR_GET, loc->ret);
  progAddChild(node, loc);
}

location(node) ::= DOT IDENTIFIER(loc1) SLASH IDENTIFIER(loc2). {
  node = progNode(prog->id++, loc1, INSTR_MEM_AREA, memoryIdentifierType(prog, node, loc1->str, loc2->str));
  // TODO: do I need this child node?
  progAddChild(node, progNode(prog->id++, loc2, INSTR_MEM_LOCATION, RET_VOID));
}

statement(node) ::= PRINT(p) OPEN_PAREN expression(expr) CLOSE_PAREN. {
  // TODO: this function is on the axe
  node = progNode(prog->id++, p, INSTR_PRINT, RET_VOID);
  progAddChild(node, expr);
}

statement(node) ::= IDENTIFIER(IDENT) EQUALS expression(expr). {
  // TODO: this is incorrect usage of ret, but emit needs this.
  node = progNode(prog->id++, IDENT, INSTR_LOCAL_ASSIGN, expr->ret);
  progAddChild(node, expr);

  int ivarId = lutFind(&prog->local_lut, node);
  if (ivarId == -1) {
    printError(prog, node, "cannot assign to undefined local variable");
  } else {
    if (expr->ret == RET_I) {
      expr->ret = prog->local_lut.types[ivarId];
      node->ret = expr->ret;
    } else if (prog->local_lut.types[ivarId] != expr->ret) {
      if (expr->type == INSTR_FUNCTION_CALL) {
        // TODO: only do this if the function is unresolved
        expr->ret = prog->local_lut.types[ivarId];
      }
    }
    node->loc = prog->local_lut.locs[ivarId];
  }
}

statement(node) ::= GLOBAL IDENTIFIER(IDENT) EQUALS expression(expr). {
  node = progNode(prog->id++, IDENT, INSTR_GLOBAL_ASSIGN, RET_VOID);
  progAddChild(node, expr);

  int ivarId = lutFind(&prog->global_lut, node);
  if (ivarId == -1) {
    printError(prog, node, "cannot assign to undefined global variable");
  } else {
    if (expr->ret == RET_I) {
      expr->ret = prog->global_lut.types[ivarId];
    } else if (prog->global_lut.types[ivarId] != expr->ret) {
      printError(prog, node, "global assigning incompatible types");
      fprintf(stderr, "%i != %i\n", prog->local_lut.types[ivarId], expr->ret);
    }
    // TODO: is this intended to be expr->ret?
    // changing the ret on an assign seems super wrong...
    // node->ret = prog->global_lut.types[ivarId];
    node->loc = prog->global_lut.locs[ivarId];
  }
}

statement(node) ::= fcall(fc). {
  node = fc;
}

statement(node) ::= THEME(th) COLOR_CODE(background) COLOR_CODE(c0) COLOR_CODE(c1) COLOR_CODE(c2). {
  prog->used_system = 1;

  node = progNode(prog->id++, th, INSTR_THEME, RET_VOID);

  progAddChild(node, progNode(prog->id++, background, INSTR_COLOR_BACKGROUND, RET_VOID));
  progAddChild(node, progNode(prog->id++, c0, INSTR_COLOR0, RET_VOID));
  progAddChild(node, progNode(prog->id++, c1, INSTR_COLOR1, RET_VOID));
  progAddChild(node, progNode(prog->id++, c2, INSTR_COLOR2, RET_VOID));
}

statement(node) ::= RETURN(RET) COLON expression(expr). {
  node = progNode(prog->id++, RET, INSTR_RETURN, expr->ret);
  progAddChild(node, expr);
}

statement(node) ::= RETURN(ret). {
  node = progNode(prog->id++, ret, INSTR_RETURN, RET_VOID);
}

statement(node) ::= WHILE(cond) expression(expr) DO_BLOCK statements(states) END_BLOCK. {
  node = progNode(prog->id++, cond, INSTR_WHILE, RET_VOID);
  progAddChild(node, expr);
  progAddChild(node, states);
  node->ret = RET_VOID;
}

statement(node) ::= IF(cond) expression(expr) DO_BLOCK statements(true_block) END_BLOCK. {
  node = progNode(prog->id++, cond, INSTR_CONDITION, true_block->ret);
  if (expr->ret != RET_I8 && expr->ret != RET_I16) {
    printError(prog, node, "Conditionals must return a number");
    fprintf(stderr, "type %i\n", expr->ret);
  }

  progAddChild(node, expr);
  progAddChild(node, true_block);
}

statement(node) ::= IF(cond) expression(expr) DO_BLOCK statements(true_block) conditional_elses(ces) END_BLOCK. {
  node = progNode(prog->id++, cond, INSTR_CONDITION, true_block->ret);

  // TODO: check true_block->ret == ces->ret

  progAddChild(node, expr);
  progAddChild(node, true_block);
  progAddChild(node, ces);
}

statement(node) ::= IF(cond) expression(expr) DO_BLOCK statements(true_block) conditional_elses(ces) ELSE statements(else_block) END_BLOCK. {
  node = progNode(prog->id++, cond, INSTR_CONDITION, true_block->ret);
  progAddChild(node, expr);
  progAddChild(node, true_block);
  progAddChild(node, ces);

  progAddChild(ces->children[ces->children_index - 1], else_block);
}

statement(node) ::= IF(cond) expression(expr) DO_BLOCK statements(true_block) ELSE statements(else_block) END_BLOCK. {
  node = progNode(prog->id++, cond, INSTR_CONDITION, true_block->ret);
  if ((true_block->ret != RET_VOID || else_block->ret != RET_VOID) && true_block->ret != else_block->ret) {
    printError(prog, node, "true and else block return different types");
  }
  progAddChild(node, expr);
  progAddChild(node, true_block);
  progAddChild(node, else_block);
}

conditional_elses(node) ::= conditional_elses(ces) conditional_else(ce). {
  progAddChild(ces, ce);
  node = ces;
}

conditional_elses(node) ::= conditional_else(ce). {
  node = ce;
}

conditional_else(node) ::= ELSE_IF(cond) expression(expr) DO_BLOCK statements(states). {
  node = progNode(prog->id++, cond, INSTR_CONDITION, RET_VOID);
  progAddChild(node, expr);
  progAddChild(node, states);
}

expression(node) ::= PAD(p) OPEN_PAREN arguments(args) CLOSE_PAREN. {
  node = progNode(prog->id++, p, INSTR_PAD, RET_I16);
  progAddChild(node, args);
}

expression(node) ::= fcall(fc). {
  node = fc;
}

arguments(node) ::= . {
  node = progEmptyNode(prog->id++, INSTR_ROOT, RET_VOID);
}

arguments(node) ::= expression(arg). {
  node = progEmptyNode(prog->id++, INSTR_ROOT, RET_VOID);
  progAddChild(node, arg);
}

arguments(node) ::= arguments(nodes) COMMA expression(arg). {
  progAddChild(nodes, arg);
  node = nodes;
}

expression(node) ::= IDENTIFIER(IDENT). {
  node = progNode(prog->id++, IDENT, INSTR_LOCAL_READ, RET_VOID);

  int ivarId = lutFind(&prog->local_lut, node);
  if (ivarId == -1) {
    printError(prog, node, "cannot find local variable");
  } else {
    node->ret = prog->local_lut.types[ivarId];
    node->loc = prog->local_lut.locs[ivarId];
  }
}

expression(node) ::= IDENTIFIER(IDENT) OPEN_SQUARE_BRACKET expression(index) CLOSE_SQUARE_BRACKET. {
  node = progNode(prog->id++, IDENT, INSTR_LOCAL_ARRAY_READ, RET_VOID);

  // TODO:
  int ivarId = lutFind(&prog->local_lut, node);
  if (ivarId == -1) {
    printError(prog, node, "cannot find local variable");
  } else {
    if (prog->local_lut.types[ivarId] == RET_I8S) {
      node->ret = RET_I8;
    } else if (prog->local_lut.types[ivarId] == RET_CHARS) {
      node->ret = RET_CHAR;
    } else if (prog->local_lut.types[ivarId] == RET_I16S) {
      node->ret = RET_I16;
    } else {
      printError(prog, node, "Attempted to index into non-array variable");
      fprintf(stderr, "lut: %i\n", prog->local_lut.types[ivarId]);
    }
    node->loc = prog->local_lut.locs[ivarId];
  }

  if (index->ret == RET_I8) {
    // insert a pad to upgrade the i8 to a i16 just for this operation
    Node * pad = progImplicitPad(prog, node);
    progAddChild(node, pad);
    progAddChild(pad, index);
  } else {
    progAddChild(node, index);
  }
}

statement(node) ::= IDENTIFIER(IDENT) OPEN_SQUARE_BRACKET expression(index) CLOSE_SQUARE_BRACKET EQUALS expression(expr). {
  node = progNode(prog->id++, IDENT, INSTR_LOCAL_ARRAY_ASSIGN, RET_VOID);
  progAddChild(node, expr);

  int ivarId = lutFind(&prog->local_lut, node);
  if (ivarId == -1) {
    printError(prog, node, "cannot assign to undefined local variable");
  } else {
    if (expr->ret == RET_I) {
      if (prog->local_lut.types[ivarId] == RET_I8S) {
        expr->ret = RET_I8;
      } else if (prog->local_lut.types[ivarId] == RET_CHARS) {
        // TODO: not sure we can reach this one since char is never ambiguous.
        expr->ret = RET_CHAR;
      } else if (prog->local_lut.types[ivarId] == RET_I16S) {
        expr->ret = RET_I16;
      } else {
        // TODO: We can also just add this to the loc
        //       That saves a node, which is nice.
        printError(prog, node, "cannot assign this type to a local variable IMPLEMENT ME");
      }
      // TODO: why did I write this this way???
    } else if (prog->local_lut.types[ivarId] == RET_I8S && expr->ret == RET_I8) {
    } else if (prog->local_lut.types[ivarId] == RET_I16S && expr->ret == RET_I16) {
    } else if (prog->local_lut.types[ivarId] == RET_CHARS && expr->ret == RET_CHAR) {
    } else if (prog->local_lut.types[ivarId] != expr->ret) {
      printError(prog, expr, "array assigning incompatible types");
      fprintf(stderr, "%i != %i\n", prog->local_lut.types[ivarId], expr->ret);
    }
    node->loc = prog->local_lut.locs[ivarId];
  }

  if (index->ret == RET_I8) {
    // insert a pad to upgrade the i8 to a i16 just for this operation
    Node * pad = progImplicitPad(prog, node);
    progAddChild(node, pad);
    progAddChild(pad, index);
  } else {
    progAddChild(node, index);
  }

  node->ret = RET_VOID;
}

expression(node) ::= GLOBAL IDENTIFIER(IDENT) OPEN_SQUARE_BRACKET expression(index) CLOSE_SQUARE_BRACKET. {
  node = progNode(prog->id++, IDENT, INSTR_GLOBAL_ARRAY_READ, RET_VOID);

  int ivarId = lutFind(&prog->global_lut, node);
  if (ivarId == -1) {
    printError(prog, node, "cannot find global variable");
  } else {
    if (prog->global_lut.types[ivarId] == RET_I8S) {
      node->ret = RET_I8;
    } else if (prog->global_lut.types[ivarId] == RET_I16S) {
      node->ret = RET_I16;
    } else {
      // TODO: We can also just add this to the loc
      //       That saves a node, which is nice.
      node->ret = prog->global_lut.types[ivarId];
    }
    node->loc = prog->global_lut.locs[ivarId];
  }

  if (index->ret == RET_I8) {
    // insert a pad to upgrade the i8 to a i16 just for this operation
    Node * pad = progImplicitPad(prog, node);
    progAddChild(node, pad);
    progAddChild(pad, index);
  } else {
    progAddChild(node, index);
  }
}

statement(node) ::= GLOBAL IDENTIFIER(IDENT) OPEN_SQUARE_BRACKET expression(index) CLOSE_SQUARE_BRACKET EQUALS expression(expr). {
  node = progNode(prog->id++, IDENT, INSTR_GLOBAL_ARRAY_ASSIGN, RET_VOID);
  progAddChild(node, expr);

  int ivarId = lutFind(&prog->global_lut, node);
  if (ivarId == -1) {
    printError(prog, node, "cannot assign to undefined global variable");
  } else {
    if (expr->ret == RET_I) {
      if (prog->global_lut.types[ivarId] == RET_I8S) {
        expr->ret = RET_I8;
      } else if (prog->global_lut.types[ivarId] == RET_I16S) {
        expr->ret = RET_I16;
      } else {
        // TODO: We can also just add this to the loc
        //       That saves a node, which is nice.
        fprintf(stderr,"UH OH\n");
      }
    } else if (prog->global_lut.types[ivarId] == RET_I8S && expr->ret == RET_I8) {
    } else if (prog->global_lut.types[ivarId] == RET_I16S && expr->ret == RET_I16) {
    } else if (prog->global_lut.types[ivarId] != expr->ret) {
      printError(prog, expr, "array assigning incompatible types");
      fprintf(stderr, "%i != %i\n", prog->global_lut.types[ivarId], expr->ret);
    }
    node->loc = prog->global_lut.locs[ivarId];
  }

  if (index->ret == RET_I8) {
    // insert a pad to upgrade the i8 to a i16 just for this operation
    Node * pad = progImplicitPad(prog, node);
    progAddChild(node, pad);
    progAddChild(pad, index);
  } else {
    progAddChild(node, index);
  }

  node->ret = RET_VOID;
}

expression(node) ::= GLOBAL IDENTIFIER(IDENT). {
  node = progNode(prog->id++, IDENT, INSTR_GLOBAL_READ, RET_VOID);

  int ivarId = lutFind(&prog->global_lut, node);
  if (ivarId == -1) {
    printError(prog, node, "cannot find global variable");
  } else {
    node->ret = prog->global_lut.types[ivarId];
    node->loc = prog->global_lut.locs[ivarId];
  }
}

expression(node) ::= OPEN_PAREN expression(expr) CLOSE_PAREN. {
  node = expr;
}


expression(node) ::= expression(lhs) TIMES(op) expression(rhs). {
  node = progNode(prog->id++, op, INSTR_TIMES, lhs->ret);

  progAddChild(node, lhs);
  progAddChild(node, rhs);

  verifyTypes(prog, node, lhs, rhs);
}

expression(node) ::= expression(lhs) SLASH(op) expression(rhs). {
  node = progNode(prog->id++, op, INSTR_DIVIDE, lhs->ret);

  progAddChild(node, lhs);
  progAddChild(node, rhs);

  verifyTypes(prog, node, lhs, rhs);
}

expression(node) ::= expression(lhs) PLUS(op) expression(rhs). {
  node = progNode(prog->id++, op, INSTR_PLUS, lhs->ret);

  progAddChild(node, lhs);
  progAddChild(node, rhs);

  verifyTypes(prog, node, lhs, rhs);
}

expression(node) ::= expression(lhs) MINUS(op) expression(rhs). {
  node = progNode(prog->id++, op, INSTR_MINUS, lhs->ret);

  progAddChild(node, lhs);
  progAddChild(node, rhs);

  verifyTypes(prog, node, lhs, rhs);
}

expression(node) ::= expression(lhs) MODULO(op) expression(rhs). {
  node = progNode(prog->id++, op, INSTR_MODULO, lhs->ret);

  progAddChild(node, lhs);
  progAddChild(node, rhs);

  verifyTypes(prog, node, lhs, rhs);
}

expression(node) ::= expression(lhs) EQUALITY(op) expression(rhs). {
  node = progNode(prog->id++, op, INSTR_EQUALITY, RET_I8);

  progAddChild(node, lhs);
  progAddChild(node, rhs);

  verifyTypes(prog, node, lhs, rhs);
}

expression(node) ::= expression(lhs) NOT_EQUALITY(op) expression(rhs). {
  node = progNode(prog->id++, op, INSTR_NOT_EQUALITY, RET_I8);

  progAddChild(node, lhs);
  progAddChild(node, rhs);

  verifyTypes(prog, node, lhs, rhs);
}


expression(node) ::= expression(lhs) GREATER_THAN(op) expression(rhs). {
  node = progNode(prog->id++, op, INSTR_GREATER_THAN, RET_I8);

  progAddChild(node, lhs);
  progAddChild(node, rhs);

  verifyTypes(prog, node, lhs, rhs);
}

expression(node) ::= expression(lhs) LESS_THAN(op) expression(rhs). {
  node = progNode(prog->id++, op, INSTR_LESS_THAN, RET_I8);

  progAddChild(node, lhs);
  progAddChild(node, rhs);

  verifyTypes(prog, node, lhs, rhs);
}

expression(node) ::= expression(lhs) EXPR_AND(op) expression(rhs). {
  node = progNode(prog->id++, op, INSTR_AND, RET_I8);

  progAddChild(node, lhs);
  progAddChild(node, rhs);

  verifyTypes(prog, node, lhs, rhs);
}

expression(node) ::= expression(lhs) EXPR_OR(op) expression(rhs). {
  node = progNode(prog->id++, op, INSTR_OR, RET_I8);

  progAddChild(node, lhs);
  progAddChild(node, rhs);

  verifyTypes(prog, node, lhs, rhs);
}

expression(node) ::= INT_LITERAL(LIT). {
  node = progNode(prog->id++, LIT, INSTR_LITERAL_INT, RET_I);
}

expression(node) ::= SINGLE_QUOTE IDENTIFIER(tok). {
  node = progNode(prog->id++, tok, INSTR_LITERAL_CHAR, RET_CHAR);
}

expression(node) ::= STRING(tok). {
  node = progNode(prog->id++, tok, INSTR_STRING_LITERAL, RET_STRING);
}

expression(node) ::= EMPTY_STRING(tok). {
  tok->str = "";
  node = progNode(prog->id++, tok, INSTR_STRING_LITERAL, RET_STRING);
}
