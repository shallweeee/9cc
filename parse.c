#include "9cc.h"

Token* token;

Node* code[100];
LVar* strings;
LVar* globals;
LVar* locals;
int locals_offset;
LVar** func_globals;
int func_globals_count;
bool in_func;

int is_alnum(char c) {
  return ('a' <= c && c <= 'z') ||
         ('A' <= c && c <= 'Z') ||
         ('0' <= c && c <= '9') ||
         ( c == '_');
}

int get_sizeof(Type* type) {
  if (type->ty == ARRAY)
    return type->array_size * get_sizeof(type->ptr_to);
  if (type->ty == PTR)
    return PTRSIZE;
  if (type->ty == CHAR)
    return 1;
  return INTSIZE;
}

LVar* find_string(Token* tok) {
  for (LVar* var = strings; var; var = var->next)
    if (var->len == tok->len && !memcmp(var->name, tok->str, tok->len))
      return var;
  return NULL;
}

LVar* find_lvar(Token* tok) {
  for (LVar* var = locals; var; var = var->next)
    if (var->len == tok->len && !memcmp(var->name, tok->str, tok->len))
      return var;
  return NULL;
}

LVar* find_gvar(Token* tok) {
  for (LVar* var = globals; var; var = var->next)
    if (var->len == tok->len && !memcmp(var->name, tok->str, tok->len))
      return var;
  return NULL;
}

Type* new_char_type();
Type* add_array_type(Type* to, int size);

LVar* new_string(Token* tok) {
  LVar* var = calloc(1, sizeof(LVar));
  var->next = strings;
  var->name = tok->str;
  var->len = tok->len;
  var->offset = strings ? strings->offset + 1 : 0;
  var->type = add_array_type(new_char_type(), tok->len + 1);
  var->string = 1;

  strings = var;
  return var;
}

LVar* new_lvar(Token* tok, Type* type) {
  LVar* lvar = calloc(1, sizeof(LVar));
  lvar->next = locals;
  lvar->name = tok->str;
  lvar->len = tok->len;
  int aligned_size = get_sizeof(type);
  aligned_size += -aligned_size & 3;
  lvar->offset = locals_offset += aligned_size;
  lvar->type = type;

  locals = lvar;
  return lvar;
}

int get_global_index(LVar* lvar) {
  for (int i = 0; i < func_globals_count; ++i) {
    if (lvar == func_globals[i])
      return i;
  }

  func_globals = (LVar**)realloc(func_globals, sizeof(LVar*) * (func_globals_count + 1));
  func_globals[func_globals_count] = lvar;
  return func_globals_count++;
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
    error_at(token->str, "It's not expected");
  token = token->next;
}

int expect_number() {
  if (token->kind != TK_NUM)
    error_at(token->str, "It's not a number");
  int val = token->val;
  token = token->next;
  return val;
}

Token* expect_kind(TokenKind kind) {
  if (token->kind != kind)
    error_at(token->str, "It's not a expected kind");
  Token* cur = token;
  token = token->next;
  return cur;
};

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

    if (strncmp(p, "//", 2) == 0) {
      p += 2;
      while (*p != '\n')
        p++;
      continue;
    }

    if (strncmp(p, "/*", 2) == 0) {
      char *q = strstr(p + 2, "*/");
      if (!q)
        error_at(p, "comment was not closed");
      p = q + 2;
      continue;
    }

    if (*p == '"') {
      cur = new_token(TK_STR, cur, ++p);
      while (*p != '"')
        p++;
      cur->len = p - cur->str;
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
        *p == ';' || *p == '=' || *p == '{' || *p == '}' ||
        *p == ',' || *p == '&' || *p == '[' || *p == ']') {
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
    CHECK_KEYWORD("for", 3, TK_FOR);
    CHECK_KEYWORD("int", 3, TK_INT);
    CHECK_KEYWORD("sizeof", 6, TK_SIZEOF);
    CHECK_KEYWORD("char", 4, TK_CHAR);

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

Type* new_int_type() {
  static Type type = {INT};
  return &type;
}

Type* new_char_type() {
  static Type type = {CHAR};
  return &type;
}

Type* add_ptr_type(Type* to) {
  Type* type = calloc(1, sizeof(Type));
  type->ty = PTR;
  type->ptr_to = to;
  return type;
}

Type* add_array_type(Type* to, int size) {
  Type* type = calloc(1, sizeof(Type));
  type->ty = ARRAY;
  type->ptr_to = to;
  type->array_size = size;
  return type;
}

Node* new_node(NodeKind kind, Node* lhs, Node* rhs) {
  Node* node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;

  switch (kind) {
    case ND_DEREF:
      node->type = rhs->type->ptr_to;
      break;
    case ND_ADDR:
      node->type = add_ptr_type(rhs->type);
      break;
    case ND_ADD: // Fall-through
    case ND_SUB:
      if (((lhs->type->ty == ARRAY) || (lhs->type->ty == PTR)) &&
          ((rhs->type->ty == ARRAY) || (rhs->type->ty == PTR)))
        error("two pointer operation");

      if (lhs->type->ty == ARRAY)
        node->type = add_ptr_type(lhs->type->ptr_to);
      else if (lhs->type->ty == PTR)
        node->type = lhs->type;
      else if (rhs->type->ty == ARRAY)
        node->type = add_ptr_type(rhs->type->ptr_to);
      else
        node->type = rhs->type;
      break;
    case ND_MUL: // Fall-through
    case ND_DIV:
      node->type = lhs->type;
      break;
    case ND_EQU:
    case ND_NEQ:
      node->type = new_int_type();
      break;
    case ND_ASSIGN:
      node->type = lhs->type;
    default:
      break;
  }

  return node;
}

Node* new_node_num(int val) {
  Node* node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  node->type = new_int_type();
  return node;
}

Node* expr();

void add_array(Node* parent, Node* child) {
  parent->array = realloc(parent->array, PTRSIZE * (parent->val + 1)); // sizeof(Node*)
  parent->array[parent->val++] = child;
}

void add_param() {
  if (!consume_kind(TK_INT))
    return;

  Type* type = new_int_type();
  while (consume("*"))
    type = add_ptr_type(type);

  Token* tok = expect_kind(TK_IDENT);

  if (find_lvar(tok))
    error("%.*s exists already", tok->len, tok->str);

  new_lvar(tok, type);
}

void optimize_param(Node* node) {
  node->params = locals_offset / PTRSIZE;
  if (node->params <= REG_PARAMS)
    return;

  LVar* param = locals;
  while (param->offset > PTRSIZE * REG_PARAMS) {
    param->offset = PTRSIZE * 3 - param->offset; // push fp, pc
    param = param->next;
  }
  locals_offset = PTRSIZE * REG_PARAMS;
}

Node* new_node_ident() {
  Token* tok = consume_kind(TK_IDENT);
  if (!tok)
    return NULL;

  if (consume("(")) {
    Node* node = calloc(1, sizeof(Node));
    node->kind = ND_CALL;
    node->token = tok;
    node->type = new_int_type(); // TBC
    if (!consume(")")) {
      do {
        add_array(node, expr());
      } while (consume(","));
      expect(")");
    }
    return node;
  }

  LVar* lvar = find_lvar(tok);
  bool global = false;
  if (!lvar) {
    lvar = find_gvar(tok);
    if (!lvar)
      error("%.*s was not defined", tok->len, tok->str);
    global = true;
  }

  Node* node = calloc(1, sizeof(Node));
  node->kind = ND_LVAR;
  node->type = lvar->type;
  node->token = tok; // debug info
  node->global = global;
  if (global)
    node->offset = get_global_index(lvar);
  else
    node->offset = lvar->offset;
  return node;
}

Node* new_node_string() {
  Token* tok = consume_kind(TK_STR);
  if (!tok)
    return NULL;

  LVar* var = find_string(tok);
  if (!var)
    var = new_string(tok);

  Node* node = calloc(1, sizeof(Node));
  node->kind = ND_STR;
  node->type = var->type;
  node->token = tok;
  node->global = true;
  node->offset = get_global_index(var);

  return node;
}

Node* primary() {
  Node* node;

  if (consume("(")) {
    node = expr();
    expect(")");
    return node;
  }

  node = new_node_ident();
  if (node)
    return node;

  node = new_node_string();
  if (node)
    return node;

  return new_node_num(expect_number());
}

Node* unary() {
  if (consume_kind(TK_SIZEOF)) {
    Node* node = unary();
    return new_node_num(get_sizeof(node->type));
  }
  if (consume("*"))
    return new_node(ND_DEREF, NULL, unary());
  if (consume("&"))
    return new_node(ND_ADDR, NULL, unary());
  if (consume("-"))
    return new_node(ND_SUB, new_node_num(0), primary());
  if (consume("+"))
    return primary();

  Node* node = primary();
  if (consume("[")) {
    node = new_node(ND_ADD, node, primary());
    expect("]");
    node = new_node(ND_DEREF, NULL, node);
  }
  return node;
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

Node* stmt_var_definition(Type* type) {
  while (consume("*"))
    type = add_ptr_type(type);

  Token* tok = expect_kind(TK_IDENT);

  if (find_lvar(tok))
    error("local %.*s exists already", tok->len, tok->str);
  if (find_gvar(tok))
    error("global %.*s exists already", tok->len, tok->str);

  int array_size = 0;
  if (consume("[")) {
    array_size = expect_number();
    expect("]");
  }

  expect(";");

  if (array_size)
    type = add_array_type(type, array_size);

  new_lvar(tok, type);

  Node* node = new_node(ND_VARIABLE, NULL, NULL);
  node->token = tok;
  node->type = type;
  node->global = !in_func;

  return node;
}

Node* stmt() {
  Node* node;

  if (consume_kind(TK_RETURN)) {
    node = calloc(1, sizeof(Node));
    node->kind = ND_RETURN;
    if (!consume(";")) {
      node->rhs = expr();
      expect(";");
    }
  } else if (consume_kind(TK_IF)) {
    expect("(");
    Node* if_expr = expr();
    expect(")");
    node = new_node(ND_IF, if_expr, stmt());
    if (consume_kind(TK_ELSE)) {
      node->array = calloc(1, PTRSIZE);
      node->array[0] = stmt();
    }
    return node;
  } else if (consume_kind(TK_WHILE)) {
    expect("(");
    Node* while_expr = expr();
    expect(")");
    return new_node(ND_WHILE, while_expr, stmt());
  } else if (consume_kind(TK_FOR)) {
    node = calloc(1, sizeof(Node));
    node->kind = ND_FOR;
    node->array = calloc(2, PTRSIZE);
    expect("(");
    if (!consume(";")) {
      node->array[0] = expr();
      expect(";");
    }
    if (!consume(";")) {
      node->lhs = expr();
      expect(";");
    }
    if (!consume(")")) {
      node->array[1] = expr();
      expect(")");
    }
    node->rhs = stmt();
    return node;
  } else if (consume("{")) {
    node = new_node(ND_BLOCK, NULL, NULL);
    while (!consume("}")) {
      add_array(node, stmt());
    }
  } else if (consume_kind(TK_INT)) {
    return stmt_var_definition(new_int_type());
  } else if (consume_kind(TK_CHAR)) {
    return stmt_var_definition(new_char_type());
  } else {
    node = expr();
    expect(";");
  }
  return node;
}

bool is_func() {
  Token* cur = token;
  bool function = consume_kind(TK_INT) && consume_kind(TK_IDENT) && consume("(");
  token = cur;
  return function;
}

Node* func() {
  if (is_func()) {
    consume_kind(TK_INT);
    Token* tok = consume_kind(TK_IDENT);
    Node* node = calloc(1, sizeof(Node));
    node->kind = ND_FUNC;
    node->token = tok;

    // hook
    in_func = true;
    globals = locals;
    locals = node->locals;
    locals_offset = 0;
    func_globals = NULL;
    func_globals_count = 0;

    // param
    expect("(");
    if (!consume(")")) {
      do {
        add_param();
      } while (consume(","));
      expect(")");
    }
    optimize_param(node);

    // body
    expect("{");
    while (!consume("}")) {
      add_array(node, stmt());
    }

    // unhook
    node->func_globals = func_globals;
    node->offset = func_globals_count;
    node->locals = locals;
    locals = globals;
    locals_offset = locals ? locals->offset : 0;
    in_func = false;

    return node;
  }

  return stmt();
}

void program() {
  int i = 0;
  while (!at_eof()) {
    code[i++] = func();
  }
  code[i] = NULL;
  globals = locals;
}

/*
 * EBNF
 * program    = func*
 * func       = stmt
 *            | "int" ident "(" ("int" "*"* ident ("," "int" "*"* ident)*)? ")" "{" stmt* "}"
 * stmt       = expr ";"
 *            | "int" "*"* ident ("[" num "]")* ";"
 *            | "{" stmt* "}"
 *            | "return" expr? ";"
 *            | "if" "(" expr ")" stmt ("else" stmt)?
 *            | "while" "(" expr ")" stmt
 *            | "for" "(" expr? ";" expr? ";" expr? ")" stmt
 * expr       = assign
 * assign     = equality ("=" assign)?
 * equality   = relational ("==" relational | "!=" relational)*
 * relational = add ("<" add | "<=" add | ">" add | ">=" add)*
 * add        = mul ("+" mul | "-" mul)*
 * mul        = unary ("*" unary | "/" unary)*
 * unary      = "sizeof" unary
 *            | ("+" | "-")? primary
 *            | "*" unary
 *            | "&" unary
 *            | primary ("[" primary "]")*
 * primary    = num
 *            | '"' string '"'
 *            | ident ("(" (expr ("," expr)*)? ")")?
 *            | "(" expr ")"
 */
