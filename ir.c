#include <stdlib.h>
#include "kcc.h"

typedef struct Env {
    Map *vars;
    struct Env *prev;
} Env;

static Vector *codes;
static int varsiz;
static int label;
static Env *env;

static Env *new_env(Env *prev) {
    Env *env = malloc(sizeof(Env));
    env->vars = new_map();
    env->prev = prev;
    return env;
}

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

static IR *add_ir_name(int ty, char *name) {
    IR *ir = add_ir(ty);
    ir->name = name;
    return ir;
}

static Var *define_var(char *name) {
    if (map_exists(env->vars, name)) {
        // TODO: analyze parse step
        error("define variable twice: %s", name);
    }
    Var *var = malloc(sizeof(Var));
    var->name = name;
    if (env->prev) {
        // local variable
        var->offset = varsiz;
        varsiz += 8;
    } else {
        // global variable
        var->offset = -1;
    }
    map_put(env->vars, name, var);
    return var;
}

static Var *find_var(char *name) {
    for (Env *e = env; e; e = e->prev) {
        if (map_exists(e->vars, name))
            return map_get(e->vars, name);
    }
    return NULL;
}

// push indicated address
static void gen_lval(Node *node) {
    Var *var;
    switch (node->ty) {
    case ND_VARDEF:
    case ND_IDENT:
        var = find_var(node->name);
        if (!var)
            error("undefined variable: %s", node->name);
        if (var->offset == -1)
            add_ir_name(IR_LABEL_ADDR, var->name);
        else
            add_ir_val(IR_PUSH_VAR_PTR, var->offset);
        break;
    default:
        error("invalid value for assign type %c (%d)", node->ty, node->ty);
    }
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
    case ND_FOR:
        gen_stmt(node->init);
        int lloop = label++;
        int lexit = label++;
        add_ir_val(IR_LABEL, lloop);
        gen_expr(node->cond);
        add_ir_val(IR_UNLESS, lexit);
        gen_stmt(node->body);
        gen_expr(node->incr);
        add_ir_val(IR_JMP, lloop);
        add_ir_val(IR_LABEL, lexit);
        return;
    case ND_VARDEF:
        define_var(node->name);
        if (node->expr) {
            gen_lval(node);
            gen_expr(node->expr);
            add_ir(IR_ASSIGN);
        }
        return;
    case ND_COMP_STMT:
        env = new_env(env);
        for (int i = 0; i < node->stmts->len; i++) {
            gen_stmt(node->stmts->data[i]);
        }
        env = env->prev;
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
    error("unexpect node for stmt: %c, %d", node->ty, node->ty);
}

static Function *gen_func(Node *node) {
    if (node->ty != ND_FUNC)
        error("toplevel must be function. but get %d", node->ty);
    Function *func = malloc(sizeof(Function));
    func->name = node->name;
    func->args = node->args->len;
    codes = new_vector();
    varsiz = 0;
    for (int i = 0; i < node->args->len; i++) {
        define_var(node->args->data[i]);
    }
    gen_stmt(node->body);
    func->codes = codes;
    func->varsiz = varsiz;
    return func;
}

Program *gen_ir(Vector *nodes) {
    env = new_env(NULL);
    Program *program = malloc(sizeof(Program));
    program->funcs = new_vector();
    for (int i = 0; i < nodes->len; i++) {
        Node *node = nodes->data[i];
        Var *var;
        switch (node->ty) {
        case ND_VARDEF:
            var = define_var(node->name);
            var->initial = node->val;
            break;
        case ND_FUNC:
            env = new_env(env);
            vec_push(program->funcs, gen_func(nodes->data[i]));
            env = env->prev;
            break;
        default:
            error("unexpected node type: %c (%d)", node->ty, node->ty);
        }
    }
    // TODO: convert global variables
    program->globals = env->vars->vals;
    return program;
}
