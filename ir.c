#include <stdlib.h>
#include <assert.h>
#include "kcc.h"

static Vector *codes;
static int label;

static IR *add_ir(int ty) {
    IR *ir = calloc(1, sizeof(IR));
    ir->ty = ty;
    vec_push(codes, ir);
    return ir;
}

static IR *add_ir_val(int ty, int val) {
    IR *ir = add_ir(ty);
    ir->val = val;
    return ir;
}

static IR *add_ir_name(int ty, char *name) {
    IR *ir = add_ir(ty);
    ir->name = name;
    return ir;
}

static void gen_expr(Node *node);

// push indicated address
static void gen_ptr(Node *node) {
    switch (node->op) {
    case ND_DEREF:
        gen_ptr(node->expr);
        add_ir(IR_LOAD_VAL);
        break;
    case ND_VARDEF:
    case ND_IDENT:
        if (node->var->offset == -1)
            add_ir_name(IR_LABEL_ADDR, node->name);
        else
            add_ir_val(IR_PUSH_VAR_PTR, node->var->offset);
        break;
    default:
        assert(0 && "can not get pointer");
    }
}

static void gen_expr(Node *node) {
    switch (node->op) {
    case ND_NUM:
        add_ir_val(IR_PUSH_IMM, node->val);
        return;
    case ND_IDENT:
        gen_ptr(node);
        add_ir(IR_LOAD_VAL);
        return;
    case ND_ADDR:
        gen_ptr(node->expr);
        return;
    case ND_DEREF:
        gen_expr(node->expr);
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
        gen_ptr(node->lhs);
        gen_expr(node->rhs);
        add_ir(IR_ASSIGN);
        return;
    case ND_LOGAND: {
        gen_expr(node->lhs);
        int lfalse = label++;
        int lexit = label++;
        add_ir_val(IR_UNLESS, lfalse);
        gen_expr(node->rhs);
        add_ir_val(IR_UNLESS, lfalse);
        add_ir_val(IR_PUSH_IMM, 1);
        add_ir_val(IR_JMP, lexit);
        add_ir_val(IR_LABEL, lfalse);
        add_ir_val(IR_PUSH_IMM, 0);
        add_ir_val(IR_LABEL, lexit);
        return;
    }
    case ND_LOGOR: {
        gen_expr(node->lhs);
        int ltrue = label++;
        int lexit = label++;
        add_ir_val(IR_IF, ltrue);
        gen_expr(node->rhs);
        add_ir_val(IR_IF, ltrue);
        add_ir_val(IR_PUSH_IMM, 0);
        add_ir_val(IR_JMP, lexit);
        add_ir_val(IR_LABEL, ltrue);
        add_ir_val(IR_PUSH_IMM, 1);
        add_ir_val(IR_LABEL, lexit);
        return;
    }
    }

    gen_expr(node->lhs);
    gen_expr(node->rhs);

    switch (node->op) {
    case '+':
    case '-':
    case '*':
    case '/':
        add_ir(node->op);
        break;
    case ND_EQ:
        add_ir(IR_EQ);
        break;
    case ND_NE:
        add_ir(IR_NE);
        break;
    default:
        assert(0 && "unknown ast node");
    }
}

static void gen_stmt(Node *node) {
    switch (node->op) {
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
    case ND_FOR:
        gen_stmt(node->init);
        int lloop = label++;
        int lexit = label++;
        add_ir_val(IR_LABEL, lloop);
        gen_expr(node->cond);
        add_ir_val(IR_UNLESS, lexit);
        gen_stmt(node->body);
        gen_expr(node->incr);
        add_ir(IR_POP);
        add_ir_val(IR_JMP, lloop);
        add_ir_val(IR_LABEL, lexit);
        return;
    case ND_VARDEF:
        if (node->expr) {
            gen_ptr(node);
            gen_expr(node->expr);
            add_ir(IR_ASSIGN);
            add_ir(IR_POP);
        }
        return;
    case ND_COMP_STMT:
        for (int i = 0; i < node->stmts->len; i++) {
            gen_stmt(node->stmts->data[i]);
        }
        return;
    case ND_EXPR_STMT:
        gen_expr(node->expr);
        add_ir(IR_POP);
        return;
    case ND_RETURN:
        gen_expr(node->expr);
        add_ir(IR_POP);
        add_ir(IR_RETURN);
        return;
    }
    assert(0 && "unknown ast node");
}

static Function *gen_func(Node *node) {
    if (node->op != ND_FUNC)
        assert(0 && "must func node");
    Function *func = calloc(1, sizeof(Function));
    func->name = node->name;
    func->args = node->args->len;
    codes = new_vector();
    gen_stmt(node->body);
    func->codes = codes;
    func->varsiz = node->varsiz;
    return func;
}

Vector *gen_ir(Vector *nodes) {
    Vector *funcs = new_vector();
    for (int i = 0; i < nodes->len; i++) {
        Node *node = nodes->data[i];
        switch (node->op) {
        case ND_VARDEF:
            break;
        case ND_FUNC:
            vec_push(funcs, gen_func(nodes->data[i]));
            break;
        default:
            assert(0 && "unknown ast node");
        }
    }
    return funcs;
}
