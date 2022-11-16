#ifndef AST
#define AST

typedef struct Token {
  int lexCode;
  char *filename;
  int line;
  int column;
  int size;
  char *str;
} Token;


typedef enum InstructionType {
  // TYPES
  INSTR_I8,
  INSTR_I16,
  INSTR_I8S,
  INSTR_I16S,
  INSTR_CHAR,
  INSTR_CHARS,
  INSTR_LITERAL_INT,
  INSTR_LITERAL_CHAR,

  // PARTS
  INSTR_ROOT,
  INSTR_BREAK,
  INSTR_MEM_AREA,
  INSTR_MEM_LOCATION,
  INSTR_LABEL,
  INSTR_PAD,
  INSTR_FUNCTION,
  INSTR_FUNCTION_CALL,
  ISNTR_ARGUMENTS,
  INSTR_STRING_LITERAL,
  INSTR_SPRITE,
  INSTR_SPRITE_ROW,

  // COLORS
  INSTR_THEME,
  INSTR_COLOR_BACKGROUND,
  INSTR_COLOR0,
  INSTR_COLOR1,
  INSTR_COLOR2,

  INSTR_LOCAL_DEFINE,
  INSTR_LOCAL_READ,
  INSTR_LOCAL_ASSIGN,
  INSTR_LOCAL_ARRAY_ASSIGN,
  INSTR_LOCAL_ARRAY_READ,

  INSTR_GLOBAL_DEFINE,
  INSTR_GLOBAL_READ,
  INSTR_GLOBAL_ASSIGN,
  INSTR_GLOBAL_ARRAY_ASSIGN,
  INSTR_GLOBAL_ARRAY_READ,

  // MATH
  INSTR_TIMES,
  INSTR_DIVIDE,
  INSTR_PLUS,
  INSTR_MINUS,
  INSTR_MODULO,

  // LOGIC
  INSTR_EQUALITY,
  INSTR_NOT_EQUALITY,
  INSTR_AND,
  INSTR_OR,
  INSTR_GREATER_THAN,
  INSTR_LESS_THAN,

  // CONTROL FLOW
  INSTR_WHILE,
  INSTR_CONDITION,
  INSTR_RETURN,

  // IO
  INSTR_GET,
  INSTR_SEND,

  // TO AXE
  INSTR_PUT,
  INSTR_PRINT
} InstructionType;

typedef enum ReturnType {
  RET_I8,
  RET_I16,
  RET_I,
  RET_I8S,
  RET_I16S,
  RET_CHAR,
  RET_CHARS,
  RET_STRING,
  RET_VOID
} ReturnType;

typedef struct Node {
  int id;
  int children_index;
  int children_capacity;
  InstructionType type;
  ReturnType ret;

  // Track if this code can leave
  // int leaves;

  int memory_address;

  int loc;
  Token *tok;
  struct Node **children;
} Node;

extern Node *progNode(int id, Token *tok, InstructionType type, ReturnType ret);
extern Node *progEmptyNode(int id, InstructionType type, ReturnType ret);
extern void progAddChild(Node *n, Node *child);

#endif
