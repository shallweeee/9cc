#include "9cc.h"

char* user_input;
Token* token;

#if defined(PRINT_TOKEN)
void print_token() {
  Token* tok = token;
  while (tok) {
    switch (tok->kind) {
      case TK_RESERVED:
        debug("RSV: %.*s", tok->len, tok->str);
        break;
      case TK_IDENT:
        debug("ID : %.*s", tok->len, tok->str);
        break;
      case TK_NUM:
        debug("NUM: %d", tok->val);
        break;
      case TK_RETURN:
        debug("RET:");
        break;
      case TK_IF:
        debug("IF :");
        break;
      case TK_ELSE:
        debug("ELS :");
        break;
      case TK_WHILE:
        debug("WHI :");
        break;
      case TK_FOR:
        debug("FOR :");
        break;
      case TK_INT:
        debug("INT :");
        break;
      case TK_EOF:
        return;
    }
    tok = tok->next;
  }
}
#endif

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "invalid parameter\n");
    return 1;
  }

  user_input = argv[1];
  tokenize(user_input);
#if defined(PRINT_TOKEN)
  print_token();
#endif
  program();
  gen_arm();

  return 0;
}
