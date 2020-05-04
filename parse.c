#include "9cc.h"

Node* code[100];
LVar* locals;

int is_alnum(char c) {
  return ('a' <= c && c <= 'z') ||
         ('A' <= c && c <= 'Z') ||
         ('0' <= c && c <= '9') ||
         ( c == '_');
}

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

LVar* find_lvar(Token* tok) {
  for (LVar* var = locals; var; var = var->next)
    if (var->len == tok->len && !memcmp(var->name, tok->str, tok->len))
      return var;
  return NULL;
}

LVar* new_lvar(Token* tok) {
  LVar* lvar = calloc(1, sizeof(LVar));
  lvar->next = locals;
  lvar->name = tok->str;
  lvar->len = tok->len;
  lvar->offset = locals ? locals->offset + 4 : 4;
  locals = lvar;
  return lvar;
}

bool consume(char* op) {
  if (token->kind != TK_RESERVED ||
      strlen(op) != token->len || memcmp(op, token->str, token->len) != 0)
    return false;
  token = token->next;
  return true;
}

Token* consume_kind(TokenKind kind) {
  if (token->kind != kind)
    return NULL;
  Token* cur = token;
  token = token->next;
  return cur;
};

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


Token* new_token(TokenKind kind, Token* cur, char* str) {
  Token* tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  cur->next = tok;
  return tok;
}

#define CHECK_KEYWORD(keyword, len, kind); \
    if (strncmp(p, keyword, len) == 0 && !is_alnum(p[len])) { \
      cur = new_token(kind, cur, p); \
      p += len; \
      continue; \
    }

void tokenize(char* p) {
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
      cur = new_token(TK_RESERVED, cur, p++);
      cur->len = 2;
      p++;
      continue;
    }

    if (*p == '+' || *p == '-' || *p == '*' || *p == '/' ||
        *p == '(' || *p == ')' || *p == '<' || *p == '>' ||
        *p == ';' || *p == '=') {
      cur = new_token(TK_RESERVED, cur, p++);
      cur->len = 1;
      continue;
    }

    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p);
      cur->val = strtol(p, &p, 10);
      continue;
    }

    CHECK_KEYWORD("return", 6, TK_RETURN);
    CHECK_KEYWORD("if", 2, TK_IF);
    CHECK_KEYWORD("else", 4, TK_ELSE);
    CHECK_KEYWORD("while", 5, TK_WHILE);

    if (isalpha(*p) || *p == '_') {
      cur = new_token(TK_IDENT, cur, p++);
      while (is_alnum(*p))
        p++;
      cur->len = p - cur->str;
      continue;
    }

    error_at(p, "Can not tokenize");
  }

  new_token(TK_EOF, cur, p);
  token = head.next;
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

Node* new_node_ident() {
  Token* tok = consume_kind(TK_IDENT);
  if (!tok)
    return NULL;

  LVar* lvar = find_lvar(tok);
  if (!lvar)
    lvar = new_lvar(tok);

  Node* node = calloc(1, sizeof(Node));
  node->kind = ND_LVAR;
  node->offset = lvar->offset;
  return node;
}

Node* primary() {
  if (consume("(")) {
    Node* node = expr();
    expect(")");
    return node;
  }

  Node* node = new_node_ident();
  if (node)
    return node;

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

Node* assign() {
  Node* node = equality();
  if (consume("="))
    node = new_node(ND_ASSIGN, node, assign());
  return node;
}

Node* expr() {
  return assign();
}

Node* stmt() {
  Node* node;

  if (consume_kind(TK_RETURN)) {
    node = new_node(ND_RETURN, NULL, expr());
    expect(";");
  } else if (consume_kind(TK_IF)) {
    expect("(");
    Node* if_expr = expr();
    expect(")");
    node = new_node(ND_IF, if_expr, stmt());
    if (consume_kind(TK_ELSE)) {
      node->epart = stmt();
    }
    return node;
  } else if (consume_kind(TK_WHILE)) {
    expect("(");
    Node* while_expr = expr();
    expect(")");
    return new_node(ND_WHILE, while_expr, stmt());
  } else {
    node = expr();
    expect(";");
  }
  return node;
}

void program() {
  int i = 0;
  while (!at_eof())
    code[i++] = stmt();
  code[i] = NULL;
}

/*
 * EBNF
 * program    = stmt*
 * stmt       = expr ";" | "return" expr ";"
                | "if" "(" expr ")" stmt ("else" stmt)?
                | "while" "(" expr ")" stmt
 * expr       = assign
 * assign     = equality ("=" assign)?
 * equality   = relational ("==" relational | "!=" relational)*
 * relational = add ("<" add | "<=" add | ">" add | ">=" add)*
 * add        = mul ("+" mul | "-" mul)*
 * mul        = unary ("*" unary | "/" unary)*
 * unary      = ("+" | "-")? primary
 * primary    = num | ident | "(" expr ")"
 */
