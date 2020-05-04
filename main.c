#include "9cc.h"

char* user_input;
Token* token;

#if defined(PRINT_TREE)
void print_tree(Node* node) {
  if (node->kind == ND_NUM)
    fprintf(stderr, " %d", node->val);
  else {
    fprintf(stderr, " (");

    const char* symbols[ND_LVAL] = {
      "+", "-", "*", "/", "==", "!=", "<", "<=",
#if defined(SUPPORT_GREATER)
      ">", ">=",
#endif
      "=",
    };
    fprintf(stderr, "%s", symbols[node->kind]);
    print_tree(node->lhs);
    print_tree(node->rhs);

    fprintf(stderr, ")");
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
  program();
  gen_arm();

  return 0;
}
