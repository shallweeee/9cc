#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "invalid parameter\n");
    return 1;
  }

  printf("  .global main\n");
  printf("main:\n");
  printf("  mov r0, #%d\n", atoi(argv[1]));
  printf("  bx lr\n");
  return 0;
}

