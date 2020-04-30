#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "invalid parameter\n");
    return 1;
  }

  char *p = argv[1];

  printf("  .global main\n");
  printf("main:\n");
  printf("  mov r0, #%d\n", strtol(p, &p, 10));

  while (*p) {
    if (*p == '+') {
      ++p;
      printf("  add r0, #%d\n", strtol(p, &p, 10));
      continue;
    }
    if (*p == '-') {
      ++p;
      printf("  sub r0, #%d\n", strtol(p, &p, 10));
      continue;
    }

    fprintf(stderr, "undefined operator '%c'\n", *p);
    return 1;
  }

  printf("  bx lr\n");
  return 0;
}
