#include "9cc.h"

#define comment printf
#define SEPARATOR printf("\n")
#define add_dummy() printf("  push {r0}\n")
#define del_dummy() printf("  pop {r0}\n")

int label_count = 0;

void gen_lval(Node* node) {
  if (node->kind != ND_LVAR && node->kind != ND_DEREF)
    error("It's neither a lvalue nor deref: %d", node->kind);

  int deref = 0;
  while (node->kind == ND_DEREF) {
    node = node->rhs;
    deref++;
  }

  printf("  sub r0, fp, #%d\n", node->offset);
  for (int i = 0; i < deref; ++i)
    printf("  ldr r0, [r0]\n");

  printf("  push {r0}\n");
}

void prologue(Node* node) {
  printf("  .global %.*s\n", node->token->len, node->token->str);
  printf("%.*s:\n", node->token->len, node->token->str);
  printf("  push {fp, lr}\n");
  printf("  mov fp, sp\n");
  if (node->locals) {
    int size = (node->locals->offset > 0) ? node->locals->offset : PTRSIZE * REG_PARAMS;
    printf("  sub sp, sp, #%d\n", size);
  }
  int rparams = node->params > REG_PARAMS ? REG_PARAMS : node->params;
  for (int i = 0; i < rparams; i++)
    printf("  str r%d, [fp, #-%d]\n", i, PTRSIZE * (i + 1));
  SEPARATOR;
}

void epilogue(Node* node) {
  printf("  mov sp, fp\n");
  printf("  pop {fp, pc}\n");
}

void print_node(Node* node) {
  switch (node->kind) {
    case ND_ADD:
      debug("node add(%d): +", node->kind);
      break;
    case ND_FUNC:
      debug("node func(%d): %.*s(%d) locals %d stmt %d", node->kind, node->token->len, node->token->str,
          node->params, node->locals ? node->locals->offset / PTRSIZE : 0, node->val);
      break;
    case ND_DEREF:
      debug("node deref(%d):", node->kind);
      break;
    case ND_VARIABLE:
      debug("node var(%d): %d* %.*s", node->kind, node->ptr_count, node->token->len, node->token->str);
      break;
    case ND_ASSIGN:
      print_node(node->lhs);
      debug("node assign(%d): =", node->kind);
      break;
    case ND_ADDR:
      debug("node addr(%d): &", node->kind);
      break;
    case ND_NUM:
      debug("node num(%d): %d", node->kind, node->val);
      break;
    case ND_LVAR:
      debug("node lvar(%d): *%d %.*s off %d", node->kind, node->ptr_count, node->token->len, node->token->str, node->offset);
      break;
    case ND_CALL:
      debug("node call(%d): %.*s(%d)", node->kind, node->token->len, node->token->str, node->val);
      break;
    default:
      debug("node %d", node->kind);
      break;
  }
}

// r0, r1
void handle_pointer_op(char* reg, int size) {
  printf("  mov r2, #%d\n", size);
  printf("  mul %s, %s, r2\n", reg, reg);
}

void handle_pointer(Node* lhs, Node* rhs) {
  if ((lhs->ptr_count > 0) && (rhs->ptr_count > 0))
    error("two pointers");

  if (rhs->ptr_count && (lhs->kind == ND_NUM))
    handle_pointer_op("r0", 4); // int, ptr
  else if (lhs->ptr_count && (rhs->kind == ND_NUM))
    handle_pointer_op("r1", 4);
}

void gen_arm_asm(Node* node) {
  print_node(node);
  switch (node->kind) {
    case ND_FUNC:
      prologue(node);

      for (int i = 0; i < node->val; ++i) {
        debug("");
        //comment("#stmt %d\n", i);
        gen_arm_asm(node->array[i]);
        del_dummy();
        SEPARATOR;
      }

      if (node->val == 0 || node->array[node->val - 1]->kind != ND_RETURN)
        epilogue(node);
      SEPARATOR;
      return;
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
      if (node->val > REG_PARAMS)
        printf("  add sp, sp, #%d\n", PTRSIZE * (node->val - REG_PARAMS));
      printf("  push {r0}\n");
      return;
    case ND_ADDR:
      gen_lval(node->rhs);
      return;
    case ND_DEREF:
      gen_arm_asm(node->rhs);
      printf("  pop {r0}\n");
      printf("  ldr r0, [r0]\n");
      printf("  push {r0}\n");
      return;
    case ND_VARIABLE:
      add_dummy();
      return;
    default:
      break;
  }

  gen_arm_asm(node->rhs);
  gen_arm_asm(node->lhs);

  printf("  pop {r0, r1}\n");

  switch (node->kind) {
    case ND_ADD:
      handle_pointer(node->lhs, node->rhs);
      printf("  add r0, r1\n");
      break;
    case ND_SUB:
      handle_pointer(node->lhs, node->rhs);
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
  // code gen
  for (int i = 0; code[i]; ++i) {
    gen_arm_asm(code[i]);
  }
}
