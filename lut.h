#ifndef LUT
#define LUT

#include "ast.h"

typedef struct Lut {
  int size;
  int index;
  int max;
  int stack_slots;
  int all_time_max;
  int search_offset;
  ReturnType *types; // TODO: rename to rets
  ReturnType **args;
  int *arg_count;
  char **names;
  int *locs;
  int defer_index;
  int defer_size;
  Node **defered_lookups;
} Lut;

extern void setupLut(Lut *lut, int size);
extern void resetLut(Lut *lut, int size);

extern int lutInsert(Lut *lut, Node *node, ReturnType t);
extern int lutFind(Lut *lut, Node *node);

extern int lutInsertFn(Lut *lut, Node *node, ReturnType t);
extern int lutFindFn(Lut *lut, Node *node);
extern int lutFindFnCall(Lut *lut, Node *node);

extern void deferLookup(Lut * lut, Node * node);

#endif
