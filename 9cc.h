#ifndef _9CC_H_
#define _9CC_H_

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PTRSIZE 4
#define REG_PARAMS 4

//#define PRINT_TREE
//#define SUPPORT_GREATER

typedef enum {
  TK_RESERVED,
  TK_IDENT,
  TK_NUM,
  TK_RETURN,
  TK_IF,
  TK_ELSE,
  TK_WHILE,
  TK_FOR,
  TK_INT,
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
  ND_IF,
  ND_WHILE,
  ND_FOR,
  ND_BLOCK,
  ND_CALL,
  ND_FUNC,
  ND_ADDR,
  ND_DEREF,
  ND_VARIABLE,
  ND_NUM,
} NodeKind;

typedef struct LVar LVar;

struct LVar {
  LVar* next;
  char* name;
  int len;
  int offset;
};

typedef struct Node Node;

struct Node {
  NodeKind kind;
  Node* lhs;
  Node* rhs;
  Node** array;
  Token* token;
  LVar* locals;
  int val;
  int offset;
  int params;
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

#if 1
#define debug(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#else
#define debug(fmt, ...) printf("#" fmt "\n", ##__VA_ARGS__)
#endif

#endif
