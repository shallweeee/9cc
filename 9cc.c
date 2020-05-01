#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define PRINT_TREE
//#define SUPPORT_GREATER

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
  ND_NUM,
} NodeKind;

typedef struct Node Node;

struct Node {
  NodeKind kind;
  Node* lhs;
  Node* rhs;
  int val;
};

typedef enum {
  TK_RESERVED,
  TK_NUM,
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

char* user_input;
Token* token;

void error(char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void error_at(char* loc, char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s^ ", pos, "");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

bool consume(char* op) {
  if (token->kind != TK_RESERVED ||
      strlen(op) != token->len || memcmp(op, token->str, token->len) != 0)
    return false;
  token = token->next;
  return true;
}

void expect(char* op) {
  if (token->kind != TK_RESERVED ||
      strlen(op) != token->len || memcmp(op, token->str, token->len) != 0)
    error_at(token->str, "It's not '%s'", op);
  token = token->next;
}

int expect_number() {
  if (token->kind != TK_NUM)
    error_at(token->str, "It's not a number");
  int val = token->val;
  token = token->next;
  return val;
}

bool at_eof() {
  return token->kind == TK_EOF;
}

Token* new_token(TokenKind kind, Token* cur, char* str, int len) {
  Token* tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

Token* tokenize(char* p) {
  Token head;
  head.next = NULL;
  Token* cur = &head;

  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    if ((*p == '=' || *p == '!' || *p == '<' || *p == '>') &&
        *(p + 1) == '=') {
      cur = new_token(TK_RESERVED, cur, p, 2);
      p += 2;
      continue;
    }
    if (*p == '+' || *p == '-' || *p == '*' || *p == '/' ||
        *p == '(' || *p == ')' || *p == '<' || *p == '>') {
      cur = new_token(TK_RESERVED, cur, p, 1);
      p++;
      continue;
    }

    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p, 0);
      cur->val = strtol(p, &p, 10);
      continue;
    }

    error_at(p, "Can not tokenize");
  }

  new_token(TK_EOF, cur, p, 0);
  return head.next;
}

Node* new_node(NodeKind kind, Node* lhs, Node* rhs) {
  Node* node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node* new_node_num(int val) {
  Node* node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  return node;
}

Node* expr();

Node* primary() {
  if (consume("(")) {
    Node* node = expr();
    expect(")");
    return node;
  }

  return new_node_num(expect_number());
}

Node* unary() {
  if (consume("-"))
    return new_node(ND_SUB, new_node_num(0), primary());

  consume("+");
  return primary();
}

Node* mul() {
  Node* node = unary();

  for (;;) {
    if (consume("*"))
      node = new_node(ND_MUL, node, unary());
    else if (consume("/"))
      node = new_node(ND_DIV, node, unary());
    else
      return node;
  }
}

Node* add() {
  Node* node = mul();

  for (;;) {
    if (consume("+"))
      node = new_node(ND_ADD, node, mul());
    else if (consume("-"))
      node = new_node(ND_SUB, node, mul());
    else
      return node;
  }
}

Node* relational() {
  Node* node = add();

  for (;;) {
    if (consume("<="))
      node = new_node(ND_LE, node, relational());
    else if (consume("<"))
      node = new_node(ND_LT, node, relational());
#if defined(SUPPORT_GREATER)
    else if (consume(">="))
      node = new_node(ND_GE, node, relational());
    else if (consume(">"))
      node = new_node(ND_GT, node, relational());
#else
    else if (consume(">="))
      node = new_node(ND_LE, relational(), node);
    else if (consume(">"))
      node = new_node(ND_LT, relational(), node);
#endif
    else
      return node;
  }
}

Node* equality() {
  Node* node = relational();

  for (;;) {
    if (consume("=="))
      node = new_node(ND_EQU, node, relational());
    else if (consume("!="))
      node = new_node(ND_NEQ, node, relational());
    else
      return node;
  }
}

Node* expr() {
  return equality();
}

/*
 * EBNF
 * expr       = equality
 * equality   = relational ("==" relational | "!=" relational)*
 * relational = add ("<" add | "<=" add | ">" add | ">=" add)*
 * add        = mul ("+" mul | "-" mul)*
 * mul        = unary ("*" unary | "/" unary)*
 * unary      = ("+" | "-")? primary
 * primary    = num | "(" expr ")"
 */

#if defined(PRINT_TREE)
void print_tree(Node* node) {
  if (node->kind == ND_NUM)
    fprintf(stderr, " %d", node->val);
  else {
    fprintf(stderr, " (");

    const char* symbols[ND_NUM] = {
      "+", "-", "*", "/", "==", "!=", "<", "<=",
#if defined(SUPPORT_GREATER)
      ">", ">=",
#endif
    };
    fprintf(stderr, "%s", symbols[node->kind]);
    print_tree(node->lhs);
    print_tree(node->rhs);

    fprintf(stderr, ")");
  }
}
#endif

Node* build() {
  Node* node = expr();
  if (!at_eof())
    error("not ended");
#if defined(PRINT_TREE)
  print_tree(node);
  fprintf(stderr, "\n");
#endif
  return node;
}

void gen_arm_asm(Node* node) {
  if (node->kind == ND_NUM) {
    printf("  mov r0, #%d\n", node->val);
    printf("  push {r0}\n");
    return;
  }

  gen_arm_asm(node->rhs);
  gen_arm_asm(node->lhs);

  printf("  pop {r0, r1}\n");

  switch (node->kind) {
    case ND_ADD:
      printf("  add r0, r1\n");
      break;
    case ND_SUB:
      printf("  sub r0, r1\n");
      break;
    case ND_MUL:
      printf("  mul r0, r1\n");
      break;
    case ND_DIV:
      printf("  bl __aeabi_idiv\n");
      break;
    case ND_EQU:
      printf("  cmp r0, r1\n");
      printf("  moveq r0, #1\n");
      printf("  movne r0, #0\n");
      break;
    case ND_NEQ:
      printf("  cmp r0, r1\n");
      printf("  moveq r0, #0\n");
      printf("  movne r0, #1\n");
      break;
    case ND_LT:
      printf("  cmp r0, r1\n");
      printf("  movlt r0, #1\n");
      printf("  movge r0, #0\n");
      break;
    case ND_LE:
      printf("  cmp r0, r1\n");
      printf("  movle r0, #1\n");
      printf("  movgt r0, #0\n");
      break;
#if defined(SUPPORT_GREATER)
    case ND_GT:
      printf("  cmp r0, r1\n");
      printf("  movgt r0, #1\n");
      printf("  movle r0, #0\n");
      break;
    case ND_GE:
      printf("  cmp r0, r1\n");
      printf("  movge r0, #1\n");
      printf("  movlt r0, #0\n");
      break;
#endif
    default:
      error("Invalid op %d", node->kind);
      break;
  }

  printf("  push {r0}\n");
}

void gen_arm(Node* node) {
  printf("  .global main\n");
  printf("main:\n");
  printf("  push {lr}\n");
  gen_arm_asm(node);
  printf("  pop {r0, pc}\n");
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "invalid parameter\n");
    return 1;
  }

  user_input = argv[1];
  token = tokenize(user_input);
  Node* node = build();
  gen_arm(node);

  return 0;
}
