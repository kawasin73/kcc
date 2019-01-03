#include <stdlib.h>
#include <assert.h>
#include "kcc.h"

typedef struct Env {
    Map *vars;
    struct Env *prev;
} Env;

static int varsiz;
static Env *env;
static Map *funcs;

static Env *new_env(Env *prev) {
    Env *e = calloc(1, sizeof(Env));
    e->vars = new_map();
    e->prev = prev;
    return e;
}

static Var *define_var(char *name, Type *ty) {
    if (map_exists(env->vars, name)) {
        // TODO: analyze parse step
        error("define variable twice: %s", name);
    }
    Var *var = calloc(1, sizeof(Var));
    var->name = name;
    var->ty = ty;
    var->siz = alloc_size(ty);
    if (env->prev) {
        // local variable
        var->offset = varsiz;
        varsiz += var->siz;
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

 static int can_assign(Node *node) {
     while (node->op == ND_DEREF)
        node = node->expr;
    return is_ptr(node->ty) || node->op == ND_IDENT;
 }

void walk(Node *node) {
    Var *var;
    switch(node->op) {
    case '=':
        walk(node->lhs);
        if (!can_assign(node->lhs))
            error("invalid value for assign type %c (%d)", node->lhs->op, node->lhs->op);
        walk(node->rhs);
        node->ty = node->lhs->ty;
        break;
    case '+':
    case '-':
        walk(node->lhs);
        walk(node->rhs);
        node->ty = node->lhs->ty;
        break;
    case '*':
    case '/':
    case ND_EQ:
    case ND_NE:
    case ND_LOGAND:
    case ND_LOGOR:
        walk(node->lhs);
        walk(node->rhs);
        node->ty = node->lhs->ty;
        if (is_ptr(node->ty))
            error("operand is not for pointer: %c (%d)", node->op, node->op);
        break;
    case ND_NUM:
        return;
    case ND_VARDEF:
        if (node->expr)
            walk(node->expr);
        var = define_var(node->name, node->ty);
        node->offset = var->offset;
        break;
    case ND_IDENT:
        var = find_var(node->name);
        if (!var)
            error("undefined variable: %s", node->name);
        node->offset = var->offset;
        node->ty = var->ty;
        break;
    case ND_ADDR:
        walk(node->expr);
        if (node->expr->op != ND_IDENT)
            error("can not take addr of int");
        node->ty = ptr_of(node->expr->ty);
        break;
    case ND_DEREF:
        walk(node->expr);
        if (!is_ptr(node->expr->ty))
            error("operand accepts only pointer");
        node->ty = node->expr->ty->ptr_of;
        break;
    case ND_IF:
        walk(node->cond);
        walk(node->then);
        if (node->els)
            walk(node->els);
        break;
    case ND_FOR:
        walk(node->init);
        walk(node->cond);
        walk(node->incr);
        walk(node->body);
        break;
    case ND_CALL: {
        Node *func = map_get(funcs, node->name);
        if (!func)
            error("undefined function: %s", node->name);
        for (int i = 0; i < node->args->len; i++) {
            Node *arg = node->args->data[i];
            walk(arg);
            Node *farg = func->args->data[i];
            if (!equal_ty(arg->ty, farg->ty))
                error("function (%s) argument (%s) is not match", node->name, farg->name);
        }
        node->ty = func->ty;
        break;
    }
    case ND_FUNC:
        map_put(funcs, node->name, node);
        varsiz = 0;
        env = new_env(env);
        for (int i = 0; i < node->args->len; i++) {
            Node *arg = node->args->data[i];
            assert(arg->op == ND_VARDEF);
            var = define_var(arg->name, arg->ty);
            arg->offset = var->offset;
        }
        walk(node->body);
        node->varsiz = varsiz;
        env = env->prev;
        break;
    case ND_EXPR_STMT:
    case ND_RETURN:
        walk(node->expr);
        // TODO: return value type check
        break;
    case ND_COMP_STMT:
        env = new_env(env);
        for (int i = 0; i < node->stmts->len; i++) {
            walk(node->stmts->data[i]);
        }
        env = env->prev;
        node->ty = NULL;
        return;
    default:
        error("unexpected node on analyze %c (%d)", node->op, node->op);
    }
}

Vector *analyze(Vector *nodes) {
    env = new_env(NULL);
    funcs = new_map();
    for (int i = 0; i < nodes->len; i++) {
        Node *node = nodes->data[i];
        Var *var;
        switch (node->op) {
        case ND_VARDEF:
            var = define_var(node->name, node->ty);
            var->initial = node->val;
            node->offset = var->offset;
            break;
        case ND_FUNC:
            walk(nodes->data[i]);
            break;
        default:
            error("unexpected node type: %c (%d)", node->op, node->op);
        }
    }
    return env->vars->vals;
}
