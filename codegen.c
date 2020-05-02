#include "9cc.h"

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
      //error("unknown op %d", node->kind);
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
