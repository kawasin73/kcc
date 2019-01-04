#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "kcc.h"

typedef struct Env {
    Map *vars;
    int endlabel;
    struct Env *prev;
} Env;

static int varsiz;
static Env *env;
static Vector *strings;
static int strlabel;
static int endlabel;
static Map *funcs;

static Env *new_env(Env *prev) {
    Env *e = calloc(1, sizeof(Env));
    e->vars = new_map();
    e->prev = prev;
    if (prev) {
        e->endlabel = prev->endlabel;
    } else {
        e->endlabel = endlabel;
    }
    return e;
}

static Literal *new_literal(Type *ty) {
    Literal *l = calloc(1, sizeof(Literal));
    l->ty = ty;
    return l;
}

static Var *new_var(char *name, Type *ty) {
    Var *var = calloc(1, sizeof(Var));
    var->name = name;
    var->ty = ty;
    var->siz = size_of(ty);
    return var;
}

static Var *define_var(char *name, Type *ty) {
    if (map_exists(env->vars, name)) {
        // TODO: analyze parse step
        error("define variable twice: %s", name);
    }
    Var *var = new_var(name, ty);
    if (env->prev) {
        // local variable
        varsiz += var->siz;
        var->offset = varsiz;
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

 static int can_call(Type *carg, Type *farg) {
    if (is_ptr(carg) && is_ptr(farg))
        return dig_ptr_of(carg) == dig_ptr_of(farg);
    return equal_ty(carg, farg);
 }

 static void assert_func(Node *a, Node *b) {
     assert(a->op == ND_FUNC || a->op == ND_FUNCDEF);
     assert(b->op == ND_FUNC || b->op == ND_FUNCDEF);
     assert((strcmp(a->name, b->name) == 0));
     if (!equal_ty(a->ty, b->ty))
        error("function return value type is not match: %s", a->name);
     if (a->args->len != b->args->len)
        error("function args size is not match: %s", a->name);
     for (int i = 0; i < a->args->len; i++) {
        Node *aarg = a->args->data[i];
        Node *barg = b->args->data[i];
        if (!equal_ty(aarg->ty, barg->ty))
            error("function (%s) argument (%s, %s) type is not match:", a->name, aarg->name, barg->name);
    }
 }

void walk(Node *node) {
    Var *var;
    Node *func;
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
    case ND_STR:
        assert(node->ty->ty == ARY && node->ty->ptr_of->ty == CHAR);
        node->name = format(".L.str%d", strlabel++);
        var = new_var(node->name, node->ty);
        var->initial = new_literal(node->ty);
        var->initial->str = node->str;
        vec_push(strings, var);
        break;
    case ND_SIZEOF:
        walk(node->expr);
        assert(node->expr->ty != NULL);
        node->ty = node->expr->ty;
        break;
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
    case ND_DO_WHILE:
        walk(node->then);
        walk(node->cond);
        break;
    case ND_CALL:
        func = map_get(funcs, node->name);
        if (!func)
            error("undefined function: %s", node->name);
        int checkarg = func && (func->args->len == node->args->len);
        for (int i = 0; i < node->args->len; i++) {
            Node *arg = node->args->data[i];
            walk(arg);
            if (checkarg) {
                // must check all args. but test.c have fprintf() which is variable argument
                Node *farg = func->args->data[i];

                if (!can_call(arg->ty, farg->ty))
                    error("function (%s) argument (%s) is not match", node->name, farg->name);
            }
        }
        node->ty = func->ty;
        break;
    case ND_FUNCDEF:
        func = map_get(funcs, node->name);
        if (func)
            assert_func(func, node);
        else
            map_put(funcs, node->name, node);
        break;
    case ND_FUNC:
        func = map_get(funcs, node->name);
        if (func) {
            if (func->op == ND_FUNC)
                error("function is already defined: %s", func->name);
            assert_func(func, node);
        }
        map_put(funcs, node->name, node);
        varsiz = 0;
        env = new_env(env);
        env->endlabel = endlabel++;
        node->endlabel = env->endlabel;
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
        walk(node->expr);
        break;
    case ND_RETURN:
        walk(node->expr);
        node->endlabel = env->endlabel;
        // TODO: return value type check
        break;
    case ND_STMT_EXPR:
        env = new_env(env);
        env->endlabel = endlabel++;
        node->endlabel = env->endlabel;
        walk(node->expr);
        env = env->prev;
        node->ty = NULL;
        break;
    case ND_COMP_STMT:
        env = new_env(env);
        for (int i = 0; i < node->stmts->len; i++) {
            walk(node->stmts->data[i]);
        }
        env = env->prev;
        node->ty = NULL;
        break;
    default:
        error("unexpected node on analyze %c (%d)", node->op, node->op);
    }
}

Program *analyze(Vector *nodes) {
    env = new_env(NULL);
    strings = new_vector();
    strlabel = 0;
    funcs = new_map();
    for (int i = 0; i < nodes->len; i++) {
        Node *node = nodes->data[i];
        Var *var;
        switch (node->op) {
        case ND_VARDEF:
            var = define_var(node->name, node->ty);
            if (node->expr) {
                Node *expr = node->expr;
                // TODO: interpreter
                var->initial = new_literal(expr->ty);
                if (expr->ty->ty == INT) {
                    var->initial->val = expr->val;
                } else {
                    assert(expr->ty->ty == ARY && expr->ty->ptr_of->ty == CHAR);
                    var->initial->str = expr->str;
                }
                if (expr->ty->ty == ARY) {
                    var->ty = expr->ty;
                }
            } else {
                var->initial = NULL;
            }
            var->is_extern = node->is_extern;
            node->offset = var->offset;
            break;
        case ND_FUNCDEF:
        case ND_FUNC:
            walk(nodes->data[i]);
            break;
        default:
            error("unexpected node type: %c (%d)", node->op, node->op);
        }
    }
    Program *program = calloc(1, sizeof(Program));
    program->globals = env->vars->vals;
    program->strs = strings;
    return program;
}
