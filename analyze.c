#include <stdlib.h>
#include "kcc.h"

typedef struct Env {
    Map *vars;
    struct Env *prev;
} Env;

static int varsiz;
static Env *env;

static Env *new_env(Env *prev) {
    Env *e = malloc(sizeof(Env));
    e->vars = new_map();
    e->prev = prev;
    return e;
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

void walk(Node *node) {
    Var *var;
    switch(node->ty) {
    case '=':
    case '+':
    case '-':
    case '*':
    case '/':
    case ND_EQ:
    case ND_NE:
    case ND_LOGAND:
    case ND_LOGOR:
        walk(node->lhs);
        walk(node->rhs);
        break;
    case ND_NUM:
        return;
    case ND_VARDEF:
        var = define_var(node->name);
        node->offset = var->offset;
        break;
    case ND_IDENT:
        var = find_var(node->name);
        if (!var)
            error("undefined variable: %s", node->name);
        node->offset = var->offset;
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
    case ND_CALL:
        for (int i = 0; i < node->args->len; i++) {
            walk(node->args->data[i]);
        }
        break;
    case ND_FUNC:
        varsiz = 0;
        env = new_env(env);
        for (int i = 0; i < node->args->len; i++) {
            define_var(node->args->data[i]);
        }
        walk(node->body);
        node->varsiz = varsiz;
        env = env->prev;
        break;
    case ND_EXPR_STMT:
    case ND_RETURN:
        walk(node->expr);
        break;
    case ND_COMP_STMT:
        env = new_env(env);
        for (int i = 0; i < node->stmts->len; i++) {
            walk(node->stmts->data[i]);
        }
        env = env->prev;
        break;
    default:
        error("unexpected node on analyze %c (%d)", node->ty, node->ty);
    }
}

Vector *analyze(Vector *nodes) {
    env = new_env(NULL);
    for (int i = 0; i < nodes->len; i++) {
        Node *node = nodes->data[i];
        Var *var;
        switch (node->ty) {
        case ND_VARDEF:
            var = define_var(node->name);
            var->initial = node->val;
            break;
        case ND_FUNC:
            walk(nodes->data[i]);
            break;
        default:
            error("unexpected node type: %c (%d)", node->ty, node->ty);
        }
    }
    return env->vars->vals;
}
