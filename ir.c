#include <stdlib.h>
#include "kcc.h"

static Vector *codes;
static int varsiz;
static int label;
static Map *vars;

static IR *add_ir(int ty) {
    IR *ir = malloc(sizeof(IR));
    ir->ty = ty;
    vec_push(codes, ir);
    return ir;
}

static IR *add_ir_val(int ty, int val) {
    IR *ir = add_ir(ty);
    ir->val = val;
    return ir;
}

// push indicated address
static void gen_lval(Node *node) {
    if (node->ty == ND_IDENT) {
        if (!map_exists(vars, node->name)) {
            map_puti(vars, node->name, varsiz);
            varsiz += 8;
        }

        int offset = map_geti(vars, node->name);
        add_ir_val(IR_PUSH_VAR_PTR, offset);
        return;
    }
    error("invalid value for assign");
}

static void gen_expr(Node *node) {
    switch (node->ty) {
    case ND_NUM:
        add_ir_val(IR_PUSH_IMM, node->val);
        return;
    case ND_IDENT:
        gen_lval(node);
        add_ir(IR_LOAD_VAL);
        return;
    case ND_CALL:
        for (int i = 0; i < node->args->len; i++) {
            gen_expr(node->args->data[i]);
        }
        IR *ir = add_ir_val(IR_CALL, node->args->len);
        ir->name = node->name;
        return;
    case '=':
        gen_lval(node->lhs);
        gen_expr(node->rhs);
        add_ir(IR_ASSIGN);
        return;
    }

    gen_expr(node->lhs);
    gen_expr(node->rhs);

    switch (node->ty) {
    case '+':
    case '-':
    case '*':
    case '/':
        add_ir(node->ty);
        break;
    case ND_EQ:
        add_ir(IR_EQ);
        break;
    case ND_NE:
        add_ir(IR_NE);
        break;
    default:
        error("unexpected ast node %d", node->ty);
    }
}

static void gen_stmt(Node *node) {
    switch (node->ty) {
    case ND_IF:
        gen_expr(node->cond);
        int x = label++;
        add_ir_val(IR_UNLESS, x);
        gen_stmt(node->then);
        if (!node->els) {
            add_ir_val(IR_LABEL, x);
            return;
        }
        int y = label++;
        add_ir_val(IR_JMP, y);
        add_ir_val(IR_LABEL, x);
        gen_stmt(node->els);
        add_ir_val(IR_LABEL, y);
        return;
    default:
        gen_expr(node);
    }
    // last statement is return value
    add_ir(IR_POP);
}

Function *gen_func(Node *node) {
    if (node->ty != ND_FUNC)
        error("toplevel must be function. but get %d", node->ty);
    Function *func = malloc(sizeof(Function));
    func->name = node->name;
    codes = new_vector();
    vars = new_map();
    varsiz = 0;
    for (int i = 0; i < node->body->len; i++) {
        gen_stmt(node->body->data[i]);
    }
    func->codes = codes;
    func->varsiz = varsiz;
    // TODO: free vars map or reuse
    return func;
}

Program *gen_ir(Vector *nodes) {
    Program *program = malloc(sizeof(Program));
    program->funcs = new_vector();
    for (int i = 0; i < nodes->len; i++) {
        vec_push(program->funcs, gen_func(nodes->data[i]));
    }
    return program;
}
