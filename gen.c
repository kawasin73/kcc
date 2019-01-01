#include <stdio.h>
#include "kcc.h"

static char *regargs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

static void gen_stmt(IR *ir) {
    switch (ir->ty) {
    case IR_PUSH_IMM:
        printf("  push %d\n", ir->val);
        return;
    case IR_PUSH_VAR_PTR:
        printf("  mov rax, rbp\n");
        printf("  sub rax, %d\n", ir->val);
        printf("  push rax\n");
        return;
    case IR_POP:
        printf("  pop rax\n");
        return;
    case IR_LOAD_VAL:
        printf("  pop rax\n");
        printf("  mov rax, [rax]\n");
        printf("  push rax\n");
        return;
    case IR_ASSIGN:
        printf("  pop rdi\n");
        printf("  pop rax\n");
        printf("  mov [rax], rdi\n");
        printf("  push rdi\n");
        return;
    case IR_LABEL:
        printf(".L%d:\n", ir->val);
        return;
    case IR_UNLESS:
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        printf("  je .L%d\n", ir->val);
        return;
    case IR_JMP:
        printf("  jmp .L%d\n", ir->val);
        return;
    case IR_CALL:
        for (int i = ir->val-1; i >= 0; i--) {
            printf("  pop %s\n", regargs[i]);
        }
        printf("  call _%s\n", ir->name);
        printf("  push rax\n");
        return;
    }

    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch (ir->ty) {
    case '+':
        printf("  add rax, rdi\n");
        break;
    case '-':
        printf("  sub rax, rdi\n");
        break;
    case '*':
        printf("  mul rdi\n");
        break;
    case '/':
        printf("  mov rdx, 0\n");
        printf("  div rdi\n");
        break;
    case IR_EQ:
        printf("  cmp rdi, rax\n");
        printf("  sete al\n");
        printf("  movzx rax, al\n");
        break;
    case IR_NE:
        printf("  cmp rdi, rax\n");
        printf("  setne al\n");
        printf("  movzx rax, al\n");
        break;
    default:
        error("unexpected IR ty %d", ir->ty);
    }

    printf("  push rax\n");
}

static void gen_func(Function *func) {
    printf("_%s:\n", func->name);
    // prologue
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    // allocate stack frame for 26 * 8 bytes
    printf("  sub rsp, %d\n", func->varsiz);

    for (int i = 0; i < func->codes->len; i++) {
        gen_stmt(func->codes->data[i]);
    }

    // epilogue
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
}

void gen(Program *program) {
    printf(".intel_syntax noprefix\n");
    printf(".global _main\n");

    for (int i = 0; i < program->funcs->len; i++) {
        gen_func(program->funcs->data[i]);
    }
}
