#include "9cc.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

char* filename;
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
      case TK_SIZEOF:
        debug("SIZEOF :");
        break;
      case TK_CHAR:
        debug("CHAR :");
        break;
      case TK_STR:
        debug("STR : %.*s", tok->len, tok->str);
        break;
      case TK_EOF:
        debug("EOF :");
        return;
    }
    tok = tok->next;
  }
}
#endif

char* read_file(char* path) {
  FILE* fp = fopen(path, "r");
  if (!fp)
    error("cannot open %s: %s", path, strerror(errno));

  if (fseek(fp, 0, SEEK_END) == -1)
    error("%s: fseek: %s", path, strerror(errno));
  size_t size = ftell(fp);
  if (fseek(fp, 0, SEEK_SET) == -1)
    error("%s: fseek: %s", path, strerror(errno));

  char* buf = calloc(1, size + 2);
  fread(buf, size, 1, fp);
  fclose(fp);

  if (size == 0 || buf[size - 1] != '\n')
    buf[size++] = '\n';
  buf[size] = '\0';

  return buf;
}

void error_at(char* loc, char* msg) {
  char* line = loc;
  while (user_input < line && line[-1] != '\n')
    line--;

  char* end = loc;
  while (*end != '\n')
    end++;

  int line_num = 1;
  for (char* p = user_input; p < line; p++)
    if (*p == '\n')
      line_num++;

  int indent = fprintf(stderr, "%s:%d: ", filename, line_num);
  fprintf(stderr, "%.*s\n", (int)(end - line), line);

  int pos = loc - line + indent;
  fprintf(stderr, "%*s", pos, "");
  fprintf(stderr, "^ %s\n", msg);

  exit(1);
}

void error(char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "invalid parameter\n");
    return 1;
  }

  filename = argv[1];
  user_input = read_file(filename);
  tokenize(user_input);
#if defined(PRINT_TOKEN)
  print_token();
#endif
  program();
  gen_arm();

  return 0;
}
