#include "9cc.h"

void gen_lval(Node* node) {
  if (node->kind != ND_LVAR)
    error("It's not a lvalue");

  printf("  sub r0, fp, #%d\n", node->offset);
  printf("  push {r0}\n");
}

void gen_arm_asm(Node* node) {
  switch (node->kind) {
    case ND_NUM:
      printf("  mov r0, #%d\n", node->val);
      printf("  push {r0}\n");
      return;
    case ND_LVAR:
      gen_lval(node);
      printf("  pop {r0}\n");
      printf("  ldr r0, [r0]\n");
      printf("  push {r0}\n");
      return;
    case ND_ASSIGN:
      gen_lval(node->lhs);
      gen_arm_asm(node->rhs);
      printf("  pop {r0, r1}\n");
      printf("  str r0, [r1]\n");
      printf("  push {r0}\n");
      return;
    default:
      break;
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
      //error("unknown op %d", node->kind);
      break;
  }

  printf("  push {r0}\n");
}

void gen_arm() {
  // prologue
  printf("  .global main\n");
  printf("main:\n");
  printf("  push {fp, lr}\n");
  printf("  mov fp, sp\n");
  printf("  sub sp, sp, #108\n");
  printf("\n");

  // code gen
  for (int i = 0; code[i]; ++i) {
    gen_arm_asm(code[i]);
    printf("  pop {r0}\n");
    printf("\n");
  }

  // epilogue
  printf("\n");
  printf("  add sp, sp, #108\n");
  printf("  pop {fp, pc}\n");
}
