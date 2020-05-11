#include "9cc.h"

#define comment printf
#define SEPARATOR printf("\n")
#define add_dummy() printf("  push {r0}\n")
#define del_dummy() printf("  pop {r0}\n")

int label_count = 0;
int func_global_label;

void gen_lval(Node* node) {
  if (node->kind != ND_LVAR)
    error("It's not a lvalue: %d", node->kind);

  if (node->global)
    printf("  ldr r0, .L%d+%d\n", func_global_label, node->offset * PTRSIZE);
  else
    printf("  sub r0, fp, #%d\n", node->offset);
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
  SEPARATOR;
  printf("  mov sp, fp\n");
  printf("  pop {fp, pc}\n");
}

void epilogue2(Node* node) {
  SEPARATOR;
  printf(".L%d:\n", func_global_label);
  for (int i = 0; i < node->offset; ++i) {
    LVar* lvar = node->func_globals[i];
    printf("  .word %.*s\n", lvar->len, lvar->name);
  }
}

char get_type_char(Type* type) {
  if (!type)
    return ' ';
  if (type->ty == INT)
    return 'I';
  if (type->ty == PTR)
    return 'P';
  if (type->ty == ARRAY)
    return 'A';
  return '?';
}

char count_locals(LVar* locals) {
  int count = 0;
  while (locals) {
    count++;
    locals = locals->next;
  }
  return count;
}

void print_node(Node* node) {
  switch (node->kind) {
    case ND_ADD:
      debug("node add %d %c : +", node->kind, get_type_char(node->type));
      break;
  //case ND_SUB:
  //case ND_MUL:
  //case ND_DIV:
  //case ND_EQU:
  //case ND_NEQ:
  //case ND_LT:
  //case ND_LE:
#if defined(SUPPORT_GREATER)
  //case ND_GT:
  //case ND_GE:
#endif
    case ND_ASSIGN:
      debug("node assign %d %c : =", node->kind, get_type_char(node->type));
      if (node->lhs->kind == ND_LVAR)
        print_node(node->lhs);
      break;
    case ND_LVAR:
      debug("node lvar %d %c : %.*s", node->kind, get_type_char(node->type), node->token->len, node->token->str);
      break;
    case ND_RETURN:
      debug("node ret %d %c :", node->kind, get_type_char(node->type));
      break;
  //case ND_IF:
  //case ND_WHILE:
  //case ND_FOR:
  //case ND_BLOCK:
    case ND_CALL:
      debug("node call %d %c : %.*s(%d)", node->kind, get_type_char(node->type), node->token->len, node->token->str, node->val);
      break;
    case ND_FUNC:
      debug("node func %d %c : %.*s(%d) locals %d stmt %d", node->kind, get_type_char(node->type), node->token->len, node->token->str,
          node->params, count_locals(node->locals), node->val);
      break;
    case ND_ADDR:
      debug("node addr %d %c : &", node->kind, get_type_char(node->type));
      print_node(node->rhs);
      break;
    case ND_DEREF:
      debug("node deref %d %c : *", node->kind, get_type_char(node->type));
      //print_node(node->rhs);
      break;
    case ND_VARIABLE: {
#if (DEBUG_LEVEL == 2)
      debug("node var %d %c : %.*s", node->kind, get_type_char(node->type), node->token->len, node->token->str);
#else
      debug2("node var %d %c : %s", node->kind, get_type_char(node->type), "int");
      Type* type = node->type;
      while (type->ty == PTR) {
        debug2("*");
        type = type->ptr_to;
      }
      debug2(" %.*s", node->token->len, node->token->str);
      if (type->ty == ARRAY)
        debug2("[%d]", type->array_size);
      debug2("\n");
#endif
      break;
    }
    case ND_NUM:
      debug("node num %d %c : %d", node->kind, get_type_char(node->type), node->val);
      break;
    default:
      debug("node %d %c :", node->kind, get_type_char(node->type));
      break;
  }
}

// r0, r1
void handle_pointer_op(char* reg, int size) {
  printf("  mov r2, #%d\n", size);
  printf("  mul %s, %s, r2\n", reg, reg);
}

void handle_pointer(Node* lhs, Node* rhs) {
  if ((lhs->type->ty == PTR) || (lhs->type->ty == ARRAY))
    handle_pointer_op("r1", (lhs->type->ptr_to->ty == INT) ? INTSIZE : PTRSIZE);
  else if ((rhs->type->ty == PTR) || (rhs->type->ty == ARRAY))
    handle_pointer_op("r0", (rhs->type->ptr_to->ty == INT) ? INTSIZE : PTRSIZE);
}

void gen_arm_asm(Node* node) {
  print_node(node);
  switch (node->kind) {
    case ND_FUNC:
      if (node->offset)
        func_global_label = label_count++;
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
      if (node->offset)
        epilogue2(node);
      SEPARATOR;
      return;
    case ND_NUM:
      printf("  mov r0, #%d\n", node->val);
      printf("  push {r0}\n");
      return;
    case ND_LVAR:
      gen_lval(node);
      if (node->type->ty != ARRAY) {
        printf("  pop {r0}\n");
        printf("  ldr r0, [r0]\n");
        printf("  push {r0}\n");
      }
      return;
    case ND_ASSIGN:
      if (node->lhs->kind == ND_DEREF)
        gen_arm_asm(node->lhs->rhs);
      else
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
      if (!node->global)
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
      if (node->type->ty == PTR)
        handle_pointer(node->lhs, node->rhs);
      printf("  add r0, r1\n");
      break;
    case ND_SUB:
      if (node->type->ty == PTR)
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

void gen_globals(LVar* var) {
  if (!var)
    return;

  printf("  .data\n");
  while (var) {
    int size = var->offset - (var->next ? var->next->offset : 0);
    printf("  .global %.*s\n", var->len, var->name);
    printf("  .size %.*s, %d\n", var->len, var->name, size);
    printf("%.*s:\n", var->len, var->name);
    for (int i = 0; i < size / 4; ++i)
      printf("  .word 0\n");

    var = var->next;
  }
}

void gen_arm() {
  gen_globals(globals);

  // code gen
  printf("  .text\n");
  for (int i = 0; code[i]; ++i) {
    gen_arm_asm(code[i]);
  }
}
