#ifndef _9CC_H_
#define _9CC_H_

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define PRINT_TREE
//#define SUPPORT_GREATER

typedef enum {
  TK_RESERVED,
  TK_IDENT,
  TK_NUM,
  TK_RETURN,
  TK_EOF,
} TokenKind;

typedef struct Token Token;
struct Token {
  TokenKind kind;
  Token* next;
  int val;
  char* str;
  int len;
};

typedef enum {
  ND_ADD,
  ND_SUB,
  ND_MUL,
  ND_DIV,
  ND_EQU,
  ND_NEQ,
  ND_LT,
  ND_LE,
#if defined(SUPPORT_GREATER)
  ND_GT,
  ND_GE,
#endif
  ND_ASSIGN,
  ND_LVAR,
  ND_RETURN,
  ND_NUM,
} NodeKind;

typedef struct Node Node;

struct Node {
  NodeKind kind;
  Node* lhs;
  Node* rhs;
  int val;
  int offset;
};

typedef struct LVar LVar;

struct LVar {
  LVar* next;
  char* name;
  int len;
  int offset;
};

extern char* user_input;
extern Token* token;
extern Node* code[100];
extern LVar* locals;

// parse.c
void error(char* fmt, ...);

void tokenize(char* p);
void program();

// codegen.c
void gen_arm();

#endif
