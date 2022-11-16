#include "compiler.h"
#include "debug.h"
#include "emit.h"
#include "lang.h"
#include "parser.h"
#include "scanner.h"
#include <stdio.h>

int main(int argc, char ** argv) {
  Program prog;
  setupCompiler(&prog);

  if (argc == 1) {
    fromStdin(&prog);
  } else if (argc == 2) {
    fromFile(&prog, argv[1]);
    prog.data.filename = argv[1];
  } else if (strcmp(argv[1], "-e") == 0) {
    fromString(&prog, argv[2]);
  } else {
    fprintf(stderr, "Can't understand command line arguments\n");
    return 1;
  }

  parse(&prog);
  build_instruction_list(&prog, prog.root);
  /* printGraph(prog.root); */
  return 0;
}
