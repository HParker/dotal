#include "debug.h"
#include "lang.h"
#include "lut.h"
#include "parser.h"
#include "scanner.h"
#include <stdio.h>

void *ParseAlloc(void *(*allocProc)(unsigned long));
void *ParseFree(void *, void (*freeProc)(void *));
void *Parse(void *, int lexCode, Token *tok, Program *data);

int id;

ReturnType InstrToRet(InstructionType instr) {
  switch (instr) {
  case INSTR_I8:
    return RET_I8;
  case INSTR_I16:
    return RET_I16;
  case INSTR_I8S:
    return RET_I8S;
  case INSTR_I16S:
    return RET_I16S;
    case INSTR_CHARS:
    return RET_CHARS;
  case INSTR_CHAR:
    return RET_CHAR;
  case INSTR_ROOT:
    return RET_VOID;
  default:
    fprintf(stderr, "failed on instr type %s\n", typeType(instr));
    return 0;
  }
}

ReturnType memoryIdentifierType(Program *prog, Node *node, char *loc1, char *loc2) {
  if (strcmp(loc1, "System") == 0) {
    prog->used_system = 1;
    if (strcmp(loc2, "vector") == 0) {
      return RET_I16;
    } else if (strcmp(loc2, "r") == 0) {
      return RET_I16;
    } else if (strcmp(loc2, "g") == 0) {
      return RET_I16;
    } else if (strcmp(loc2, "b") == 0) {
      return RET_I16;
    } else {
      printError(prog, node, "unknown aspect of System");
      fprintf(stderr, "%s\n", loc2);
      return RET_VOID;
    }
  } else if (strcmp(loc1, "Console") == 0) {
    prog->used_console = 1;
    if (strcmp(loc2, "vector") == 0) {
      return RET_I16;
    } else if (strcmp(loc2, "write") == 0) {
      return RET_I8;
    } else {
      printError(prog, node, "unknown aspect of Console");
      fprintf(stderr, "%s\n", loc2);
      return RET_VOID;
    }
  } else if (strcmp(loc1, "Screen") == 0) {
    prog->used_screen = 1;
    if (strcmp(loc2, "vector") == 0) {
      return RET_I16;
    } else if (strcmp(loc2, "width") == 0) {
      return RET_I16;
    } else if (strcmp(loc2, "height") == 0) {
      return RET_I16;
    } else if (strcmp(loc2, "x") == 0) {
      return RET_I16;
    } else if (strcmp(loc2, "y") == 0) {
      return RET_I16;
    } else if (strcmp(loc2, "addr") == 0) {
      return RET_I16;
    } else if (strcmp(loc2, "pixel") == 0) {
      return RET_I8;
    } else if (strcmp(loc2, "sprite") == 0) {
      return RET_I8;
    } else {
      printError(prog, node, "unknown aspect of Screen");
      fprintf(stderr, "%s\n", loc2);
      return RET_VOID;
    }
  } else if (strcmp(loc1, "Audio0") == 0 ||
	     strcmp(loc1, "Audio1") == 0 ||
	     strcmp(loc1, "Audio2") == 0 ||
	     strcmp(loc1, "Audio3") == 0
	     ) {
    prog->used_audio = 1;
    if (strcmp(loc2, "addr") == 0) {
      return RET_I16;
    } else if (strcmp(loc2, "length") == 0) {
      return RET_I16;
    } else if (strcmp(loc2, "adsr") == 0) {
      return RET_I16;
    } else if (strcmp(loc2, "volume") == 0) {
      return RET_I8;
    } else if (strcmp(loc2, "pitch") == 0) {
      return RET_I8;
    }
  } else if (strcmp(loc1, "Mouse") == 0) {
    prog->used_mouse = 1;
    if (strcmp(loc2, "vector") == 0) {
      return RET_I16;
    } else if (strcmp(loc2, "x") == 0) {
      return RET_I16;
    } else if (strcmp(loc2, "y") == 0) {
      return RET_I16;
    } else if (strcmp(loc2, "state") == 0) {
      return RET_I8;
    } else {
      printError(prog, node, "unknown aspect of Mouse");
      fprintf(stderr, "%s\n", loc2);
      return RET_VOID;
    }
  } else if (strcmp(loc1, "DateTime") == 0) {
    prog->used_datetime = 1;
    if (strcmp(loc2, "year") == 0) {
      return RET_I16;
    } else if (strcmp(loc2, "month") == 0) {
      return RET_I8;
    } else if (strcmp(loc2, "hour") == 0) {
      return RET_I8;
    } else if (strcmp(loc2, "minute") == 0) {
      return RET_I8;
    } else if (strcmp(loc2, "second") == 0) {
      return RET_I8;
    } else {
      printError(prog, node, "unknown aspect of DateTime");
      fprintf(stderr, "%s\n", loc2);
      return RET_VOID;
    }
  } else {
    printError(prog, node, "unknown label");
    return RET_VOID;
  }
  return RET_VOID;
}

void verifyTypes(Program * prog, Node * node, Node * lhs, Node * rhs) {
  if (rhs->ret == RET_I && lhs->ret == RET_I) {
    fprintf(stderr, "Warn: Inferring i8 type due to lack of local clearity\n");
    lhs->ret = RET_I8;
    rhs->ret = RET_I8;
  }

  if (lhs->ret == RET_I && rhs->ret != RET_I) {
    lhs->ret = rhs->ret;
  }

  if (rhs->ret == RET_I && lhs->ret != RET_I) {
    rhs->ret = lhs->ret;
  }

  if (lhs->ret != rhs->ret) {
    printError(prog, node, "expression is operating on different types");
    fprintf(stderr, "%i != %i\n", lhs->ret, rhs->ret);
  }
}

Node *progImplicitPad(Program * prog, Node * node) {
  return progNode(prog->id++, node->tok, INSTR_PAD, RET_VOID);
}


void parse(Program *prog) {
  // TODO: we can inline these variables

  yyscan_t scanner = prog->scanner;
  void *parser = prog->parser;
  ExtraData *data = &prog->data;
  int lexCode;
  Token *tok = malloc(sizeof(Token));
  do {
    lexCode = yylex(scanner);

    tok->filename = data->filename;
    // TODO: What the heck is going on here.
    //       I think a string I am getting from yyget_text is not null terminated?
    //       This is casuing some really weird behavior, but it seems super rare?
    //       right now I can only reproduce it using piano.dt for
    //       send(.Screen/addr, :sprite_key_right)
    //       that label includes the closing paren some how.
    tok->str = malloc(sizeof(char) * yyget_leng(scanner) + 1);
    tok->str[yyget_leng(scanner)] = '\0';
    tok->line = data->line;
    tok->column = data->col;
    tok->size = data->size;
    tok->lexCode = lexCode;

    strcpy(tok->str, yyget_text(scanner));

    prog->tok = tok;
    /* fprintf(stderr, "CODE: %i tok: \"%s\" :%i\n", lexCode, tok->str, tok->line); */
    Parse(parser, lexCode, tok, prog);
    tok = malloc(sizeof(Token));
  } while (lexCode > 0);

  // resolve defered lookups
  for (int i = 0; i < prog->function_lut.defer_index; i++) {
    Node * node = prog->function_lut.defered_lookups[i];
    int functionId = lutFindFnCall(&prog->function_lut, node);

    if (functionId == -1) {
      printError(prog, node, "function not found");
    } else {
      node->ret = prog->function_lut.types[functionId];
    }
  }
}

void setupCompiler(Program * prog) {
  prog->id = 0;
  prog->errored = 0;
  prog->strings.capacity = 100;
  prog->strings.index = 0;
  prog->strings.nodes = malloc(sizeof(Node *) * prog->strings.capacity);

  setupLut(&prog->local_lut, 10);
  prog->local_lut.stack_slots = MAX_STACK_SLOTS;
  setupLut(&prog->global_lut, 10);
  setupLut(&prog->function_lut, 10);

  prog->used_system = 0;
  prog->used_console = 0;
  prog->used_screen = 0;
  prog->used_mouse = 0;
  prog->used_datetime = 0;
  prog->used_print = 0;
  prog->used_audio = 0;
  prog->used_sine = 0;

  prog->root = progEmptyNode(prog->id++, INSTR_ROOT, RET_VOID);

  prog->data.line = 1;
  prog->data.col = 1;
  prog->data.size = 1;

  yylex_init_extra(&prog->data, &prog->scanner);

  prog->parser = ParseAlloc(malloc);
  prog->output = stdout;
}

void resetCompiler(Program * prog) {
  prog->id = 0;
  prog->errored = 0;
  prog->strings.capacity = 100;
  prog->strings.index = 0;

  free(prog->strings.nodes);
  prog->strings.nodes = malloc(sizeof(Node *) * prog->strings.capacity);

  resetLut(&prog->local_lut, 10);
  resetLut(&prog->global_lut, 10);
  resetLut(&prog->function_lut, 10);


  prog->used_system = 0;
  prog->used_console = 0;
  prog->used_screen = 0;
  prog->used_mouse = 0;
  prog->used_datetime = 0;
  prog->used_print = 0;
  prog->used_audio = 0;
  prog->used_sine = 0;

  prog->data.line = 1;
  prog->data.col = 1;
  prog->data.size = 1;

}

void fromString(Program * prog, char * str) {
  yy_scan_string(str, prog->scanner);
}

void fromStdin(Program * prog) {
  yyset_in(stdin, prog->scanner);
}

void toFile(Program * prog, char * file) {
  FILE * f = fopen(file, "w");

  if (!f) {
    fprintf(stderr, "could not open %s\n", file);
  }

  prog->output = f;
}

void fromFile(Program * prog, char * file) {
  FILE * f = fopen(file, "r");

  if (!f) {
    fprintf(stderr, "could not open %s\n", file);
  }

  prog->data.filename = file;
  yyset_in(f, prog->scanner);
}
