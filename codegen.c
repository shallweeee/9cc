#include "9cc.h"

#define comment printf
#define SEPARATOR printf("\n")
#define add_dummy() printf("  push {r0}\n")
#define del_dummy() printf("  pop {r0}\n")

int label_count = 0;

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
    case ND_RETURN:
      if (node->rhs) {
        gen_arm_asm(node->rhs);
        printf("  pop {r0}\n");
      }
      printf("  mov sp, fp\n");
      printf("  pop {fp, pc}\n");
      return;
    case ND_IF: {
      comment("#if\n");
      int cur_count = label_count++;
      gen_arm_asm(node->lhs);
      printf("  pop {r0}\n");
      printf("  cmp r0, #0\n");
      if (node->array)
        printf("  beq .Lelse%03d\n", cur_count);
      else
        printf("  beq .Lend%03d\n", cur_count);
      gen_arm_asm(node->rhs);
      del_dummy();
      if (node->array) {
        printf("  b .Lend%03d\n", cur_count);
        printf(".Lelse%03d:\n", cur_count);
        gen_arm_asm(node->array[0]);
        del_dummy();
      }
      printf(".Lend%03d:\n", cur_count);
      add_dummy();
      return;
    }
    case ND_WHILE: {
      comment("#while\n");
      int cur_count = label_count++;
      printf(".Lwhile%03d:\n", cur_count);
      gen_arm_asm(node->lhs);
      printf("  pop {r0}\n");
      printf("  cmp r0, #0\n");
      printf("  beq .Lend%03d\n", cur_count);
      gen_arm_asm(node->rhs);
      del_dummy();
      printf("  b .Lwhile%03d\n", cur_count);
      printf(".Lend%03d:\n", cur_count);
      add_dummy();
      return;
    }
    case ND_FOR: {
      comment("#for\n");
      int cur_count = label_count++;
      if (node->array[0]) {
        gen_arm_asm(node->array[0]);
        del_dummy();
      }
      printf(".Lfor%03d:\n", cur_count);
      if (node->lhs) {
        gen_arm_asm(node->lhs);
        printf("  pop {r0}\n");
        printf("  cmp r0, #0\n");
        printf("  beq .Lend%03d\n", cur_count);
      }
      gen_arm_asm(node->rhs);
      del_dummy();
      if (node->array[1]) {
        gen_arm_asm(node->array[1]);
        del_dummy();
      }
      printf("  b .Lfor%03d\n", cur_count);
      printf(".Lend%03d:\n", cur_count);
      add_dummy();
      return;
    }
    case ND_BLOCK:
      comment("#block\n");
      for (int i = 0; i < node->val; ++i) {
        comment("#stmt %d\n", i);
        gen_arm_asm(node->array[i]);
        del_dummy();
      }
      add_dummy();
      return;
    case ND_CALL:
      if (node->val > 0) {
        for (int i = 0; i < node->val; ++i) {
          gen_arm_asm(node->array[node->val - 1 - i]);
        }
        if (node->val == 1)
          printf("  pop {r0}\n");
        else if (node->val == 2)
          printf("  pop {r0-r1}\n");
        else if (node->val == 3)
          printf("  pop {r0-r2}\n");
        else
          printf("  pop {r0-r3}\n");
      }
      printf("  bl %.*s\n", node->token->len, node->token->str);
      if (node->val > 4)
        printf("  add sp, sp, #%d\n", PTRSIZE * (node->val - 4));
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
  if (locals)
    printf("  sub sp, sp, #%d\n", locals->offset);
  SEPARATOR;

  // code gen
  for (int i = 0; code[i]; ++i) {
    gen_arm_asm(code[i]);
    printf("  pop {r0}\n");
    SEPARATOR;
  }

  // epilogue
  SEPARATOR;
  if (locals)
    printf("  add sp, sp, #%d\n", locals->offset);
  printf("  pop {fp, pc}\n");
}
