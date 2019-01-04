#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "kcc.h"

static char *regname(int siz, int idx) {
    // URL: https://www.sigbus.info/compilerbook/#整数レジスタの一覧
    static char *lreg[] = {"al", "dil", "sil", "dl", "cl", "r8b", "r9b"};
    static char *rreg[] = {"rax", "rdi", "rsi", "rdx", "rcx", "r8", "r9"};
    assert(idx >= 0 && idx <= 6);
    switch (siz) {
    case 1:
        return lreg[idx];
    case 8:
        return rreg[idx];
    default:
        debug("size %d", siz);
        assert(0 && "unexpected reg size");
    }
}

static void fill_zero(int siz, int idx) {
    if (siz == 8)
        return;
    printf("  movzx %s, %s\n", regname(8, idx), regname(siz, idx));
}

static char *label_end(int i) {
    return format(".Lend%d", i);
}

static void gen_stmt(IR *ir) {
    switch (ir->op) {
    case IR_PUSH_IMM:
        printf("  push %d\n", ir->val);
        return;
    case IR_PUSH_VAR_PTR:
        printf("  mov rax, rbp\n");
        printf("  sub rax, %d\n", ir->val);
        printf("  push rax\n");
        return;
    case IR_PUSH:
        printf("  push %s\n", regname(8, 0));
        return;
    case IR_POP:
        printf("  pop %s\n", regname(8, 0));
        return;
    case IR_ADDR_LABEL:
        // TODO: Mach-O only. change to ELF compatible
        printf("  mov rax, [%s@GOTPCREL + rip]\n", ir->name);
        printf("  push rax\n");
        return;
    case IR_ADDR_GLOBAL:
        // TODO: Mach-O only. change to ELF compatible
        printf("  mov rax, [_%s@GOTPCREL + rip]\n", ir->name);
        printf("  push rax\n");
        return;
    case IR_LOAD_VAL:
        printf("  pop rax\n");
        printf("  mov %s, [rax]\n", regname(ir->siz, 0));
        fill_zero(ir->siz, 0);
        printf("  push rax\n");
        return;
    case IR_ASSIGN:
        printf("  pop %s\n", regname(8, 1));
        printf("  pop rax\n");
        printf("  mov [rax], %s\n", regname(ir->siz, 1));
        printf("  push %s\n", regname(8, 1));
        return;
    case IR_LABEL:
        printf(".L%d:\n", ir->val);
        return;
    case IR_LABEL_END:
        printf("%s:\n", label_end(ir->val));
        return;
    case IR_IF:
        printf("  pop %s\n", regname(8, 1));
        printf("  cmp %s, 0\n", regname(8, 1));
        printf("  jne .L%d\n", ir->val);
        return;
    case IR_UNLESS:
        printf("  pop %s\n", regname(8, 1));
        printf("  cmp %s, 0\n", regname(8, 1));
        printf("  je .L%d\n", ir->val);
        return;
    case IR_JMP:
        printf("  jmp .L%d\n", ir->val);
        return;
    case IR_CALL:
        for (int i = ir->val; i > 0; i--) {
            printf("  pop %s\n", regname(8, i));
        }
        printf("  push rbp\n");
        printf("  push rsp\n");
        printf("  call _%s\n", ir->name);
        printf("  pop rsp\n");
        printf("  pop rbp\n");
        printf("  push %s\n", regname(8, 0));
        return;
    case IR_RETURN:
        printf("  jmp %s\n", label_end(ir->val));
        return;
    }

    printf("  pop %s\n", regname(8, 1));
    printf("  pop %s\n", regname(8, 0));

    switch (ir->op) {
    case '+':
        printf("  add %s, %s\n", regname(8, 0), regname(8, 1));
        break;
    case '-':
        printf("  sub %s, %s\n", regname(8, 0), regname(8, 1));
        break;
    case '*':
        printf("  mul %s\n", regname(8, 1));
        break;
    case '/':
        printf("  mov rdx, 0\n");
        printf("  div %s\n", regname(8, 1));
        break;
    case IR_EQ:
        printf("  cmp %s, %s\n", regname(8, 1), regname(8, 0));
        printf("  sete %s\n", regname(1, 0));
        fill_zero(1, 0);
        break;
    case IR_NE:
        printf("  cmp %s, %s\n", regname(8, 1), regname(8, 0));
        printf("  setne %s\n", regname(1, 0));
        fill_zero(1, 0);
        break;
    default:
        assert(0 && "unknown ir");
    }

    printf("  push %s\n", regname(8, 0));
}

static void gen_func(Function *func) {
    printf("_%s:\n", func->name);
    // prologue
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    // set arguments from register to stack
    for (int i = 0; i < func->args->len; i++) {
        IR *ir = func->args->data[i];
        printf("  mov rax, rbp\n");
        printf("  sub rax, %d\n", ir->val);
        printf("  mov [rax], %s\n", regname(ir->siz, i+1));
    }
    // allocate stack frame
    // stack size must be aligned 16bytes.
    printf("  sub rsp, %d\n", ((func->varsiz + 15)/16)*16);

    for (int i = 0; i < func->codes->len; i++) {
        gen_stmt(func->codes->data[i]);
    }
    // TODO: detect "return" is not called
    // set return value 0 for no return
    printf("  mov %s, 0\n", regname(8, 0));
    // epilogue
    printf("%s:\n", label_end(func->endlabel));
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
}

char *backslash_escape(char *str, int len) {
    StringBuilder *sb = new_sb();
    for (int i = 0; i < len; i++) {
        static char escape_table[256] = {
            ['\b'] = 'b', ['\f'] = 'f', ['\n'] = 'n',
            ['\r'] = 'r', ['\t'] = 't', ['"'] = '"',
            ['\\'] = '\\',
        };
        if (escape_table[(int)str[i]]) {
            sb_add(sb, '\\');
            sb_add(sb, escape_table[(int)str[i]]);
        } else {
            sb_add(sb, str[i]);
        }
    }
    // return str;
    return sb_string(sb);
}

void gen_literal(Literal *l) {
    if (l->ty->ty == ARY) {
        assert(l->ty->ptr_of->ty == CHAR);
        printf("  .asciz \"%s\"\n", backslash_escape(l->str, strlen(l->str)));
        return;
    }
    switch (size_of(l->ty)) {
    case 1:
        printf("  .byte %d\n", l->val);
        break;
    case 2:
        printf("  .short %d\n", l->val);
        break;
    case 4:
        printf("  .long %d\n", l->val);
        break;
    case 8:
        printf("  .quad %d\n", l->val);
        break;
    default:
        assert(0 && "invalid size");
    }
}

void gen(Vector *globals, Vector *strs, Vector *funcs) {
    printf(".intel_syntax noprefix\n");
    printf(".data\n");
    for (int i = 0; i < globals->len; i++) {
        Var *var = globals->data[i];
        if (var->is_extern)
            continue;
        if (var->initial) {
            printf("_%s:\n", var->name);
            gen_literal(var->initial);
        } else {
            printf(".comm _%s, %d\n", var->name, size_of(var->ty));
        }
    }
    printf(".cstring\n");
    for (int i = 0; i < strs->len; i++) {
        Var *str = strs->data[i];
        assert(str->ty->ty == ARY && str->ty->ptr_of->ty == CHAR);
        assert(str->initial->ty->ty == ARY && str->initial->ty->ptr_of->ty == CHAR);
        printf("%s:\n", str->name);
        gen_literal(str->initial);
    }
    printf(".text\n");
    printf(".global _main\n");

    for (int i = 0; i < funcs->len; i++) {
        gen_func(funcs->data[i]);
    }
}
