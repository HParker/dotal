#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lang.h"
#include "lut.h"
#include "ast.h"
#include "compiler.h"
#include "debug.h"

void setupLut(Lut *lut, int size) {
  lut->index = 0;
  lut->max = 0;
  lut->stack_slots = 0;
  lut->all_time_max = 0;
  lut->search_offset = 0;
  lut->size = size;

  lut->names = malloc(sizeof(char *) * lut->size);
  lut->types = malloc(sizeof(ReturnType) * lut->size);
  lut->args = malloc(sizeof(ReturnType *) * lut->size);
  lut->arg_count = malloc(sizeof(int) * lut->size);
  lut->locs = malloc(sizeof(int) * lut->size);

  lut->defer_index = 0;
  lut->defer_size = lut->size;

  lut->defered_lookups = malloc(sizeof(Node*) * lut->defer_size);
}

void resetLut(Lut *lut, int size) {
  lut->index = 0;
  lut->max = 0;
  lut->all_time_max = 0;
  lut->search_offset = 0;
  lut->size = size;

  free(lut->names);
  free(lut->types);
  free(lut->args);
  free(lut->arg_count);
  free(lut->locs);

  lut->names = malloc(sizeof(char *) * lut->size);
  lut->types = malloc(sizeof(ReturnType) * lut->size);
  lut->args = malloc(sizeof(ReturnType *) * lut->size);
  lut->arg_count = malloc(sizeof(int) * lut->size);
  lut->locs = malloc(sizeof(int) * lut->size);

  lut->defer_index = 0;
  lut->defer_size = lut->size;

  free(lut->defered_lookups);
  lut->defered_lookups = malloc(sizeof(Node*) * lut->defer_size);
}


int lutFind(Lut *lut, Node *node) {
  if (node->memory_address != -1) {
    return node->memory_address;
  }

  int start = 0;
  if (lut->search_offset > 0) {
    start = lut->search_offset;
  }

  int found = -1;

  for (int i = start; i < lut->index; i++) {
    if (strcmp(lut->names[i], node->tok->str) == 0) {
      node->memory_address = i;
      return i;
    }
  }


  return found;
}

int lutInsert(Lut *lut, Node *node, ReturnType t) {
  int id = lutFind(lut, node);
  if (id != -1) {
    fprintf(stderr, "Warn inserting already initialized variable\n");
    return id;
  }

  if (lut->stack_slots > 0) {
    lut->names[lut->index] = node->tok->str;
    lut->types[lut->index] = t;
    lut->locs[lut->index] = lut->stack_slots * -1;
    lut->stack_slots--;

    id = lut->index++;
    return id;
  }

  if (lut->index == lut->size) {
    lut->size += 10;
    lut->names = realloc(lut->names, sizeof(char *) * lut->size);
    lut->types = realloc(lut->types, sizeof(ReturnType) * lut->size);
    lut->args = realloc(lut->args, sizeof(ReturnType *) * lut->size);
    lut->arg_count = realloc(lut->args, sizeof(int) * lut->size);
    lut->locs = realloc(lut->locs, sizeof(int) * lut->size);
  }

  lut->names[lut->index] = node->tok->str;
  lut->types[lut->index] = t;

  if (t == RET_I8 || t == RET_CHAR) {
    lut->locs[lut->index] = lut->max;
    lut->max += 1;
  } else if (t == RET_I16 || t == RET_CHARS) {
    lut->locs[lut->index] = lut->max;
    lut->max += 2;
  } else if (t == RET_I8S) {
    lut->locs[lut->index] = lut->max;
    lut->max += atoi(node->children[0]->children[0]->tok->str);
  } else if (t == RET_I16S) {
    lut->locs[lut->index] = lut->max;
    lut->max += 2 * atoi(node->children[0]->children[0]->tok->str);
  } else if (t == RET_I) {
    fprintf(stderr, "unresolved type in lut.\n");
  } else if (t == RET_VOID) {
    // TODO: this probably is a problem,
    //       but we need this for inserting function definitions
    fprintf(stderr, "bad type in lut void\n");
  } else {
    fprintf(stderr, "bad type in lut %s\n", retType(t));
  }
  node->memory_address = id;
  id = lut->index++;
  return id;
}

int lutFindFn(Lut * lut, Node *node) {
  int found = 0;
  for (int i = 0; i < lut->index ; i++) {
    if (strcmp(lut->names[i], node->tok->str) != 0) {
      continue;
    }
    if (node->ret != lut->types[i]) {
      continue;
    }

    if (lut->arg_count[i] != node->children[0]->children_index) {
      continue;
    }

    found = 1;
    for (int j = 0; j < node->children[0]->children_index; j++) {
      if (InstrToRet(node->children[0]->children[j]->children[0]->type) != lut->args[i][j]) {
        found = 0;
      }
    }
    if (found) {
      return i;
    }
  }
  return -1;
}

int lutFindFnCall(Lut * lut, Node *node) {
  int found = 0;
  for (int i = 0; i < lut->index ; i++) {
    if (strcmp(lut->names[i], node->tok->str) != 0) {
      continue;
    }

    if (node->ret != lut->types[i]) {
      continue;
    }

    if (lut->arg_count[i] != node->children[0]->children_index) {
      continue;
    }

    found = 1;
    for (int j = 0; j < node->children[0]->children_index; j++) {
      if (node->children[0]->children[j]->ret != lut->args[i][j] &&
          node->children[0]->children[j]->ret != RET_I) {
        found = 0;
      }
    }

    if (found) {
      // Finalize the types for any RET_I
      for (int j = 0; j < node->children[0]->children_index; j++) {
        if (node->children[0]->children[j]->ret == RET_I) {
          node->children[0]->children[j]->ret = lut->args[i][j];
        }
      }
      return i;
    }
  }
  return -1;
}

int lutInsertFn(Lut *lut, Node *node, ReturnType t) {
  int id = lutFindFn(lut, node);
  if (id != -1) {
    fprintf(stderr, "Warning: function insert already found %s\n", node->tok->str);
    return id;
  }

  if (lut->index == lut->size) {
    lut->size += 10;
    lut->names = realloc(lut->names, sizeof(char *) * lut->size);
    lut->types = realloc(lut->types, sizeof(ReturnType) * lut->size);
    lut->args = realloc(lut->args, sizeof(ReturnType *) * lut->size);
    lut->arg_count = realloc(lut->args, sizeof(int) * lut->size);
    lut->locs = realloc(lut->locs, sizeof(int) * lut->size);
  }

  lut->names[lut->index] = node->tok->str;
  lut->types[lut->index] = t;
  lut->index++;
  return id;
}

void deferLookup(Lut * lut, Node * node) {
  if (lut->defer_index == lut->defer_size) {
    lut->defer_size += 10;
    lut->defered_lookups = realloc(lut->defered_lookups, sizeof(Node*) * lut->defer_size);
  }
  lut->defered_lookups[lut->defer_index++] = node;
}
