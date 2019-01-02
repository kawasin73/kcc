#include <stdlib.h>
#include "kcc.h"

static Vector *tokens;
static int pos;

static Node *new_node() {
    return malloc(sizeof(Node));
}

static Node *new_binop(int op, Node *lhs, Node *rhs) {
    Node *node = new_node();
    node->op = op;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Token *next() {
    return tokens->data[pos++];
}

static Token *peek() {
    return tokens->data[pos];
}

// returns true (1) or false (0)
static int consume(int c) {
    Token *t = tokens->data[pos];
    if (t->ty != c) {
        return 0;
    }
    pos++;
    return 1;
}

static void expect(int ty) {
    Token *t = next();
    if (t->ty != ty)
        error("expected %c (%d), but got %c (%d): %s", ty, ty, t->ty, t->ty, t->input);
}

static int is_typename() {
    Token *t = peek();
    return t->ty == TK_INT;
}

static Node *assign();
static Node *compound_stmt();

static Node *term() {
    Token *t = next();
    Node *node;
    if (t->ty == '(') {
        node = assign();
        expect(')');
        return node;
    }
    node = new_node();
    if (t->ty == TK_NUM) {
        node->op = ND_NUM;
        node->val = t->val;
        return node;
    }
    if (t->ty == TK_IDENT) {
        node->name = t->name;
        if (!consume('(')) {
            node->op = ND_IDENT;
            return node;
        }
        node->op = ND_CALL;
        node->args = new_vector();
        if (consume(')'))
            return node;
        for (int i = 0; i < 6; i++) {
            vec_push(node->args, assign());
            if (consume(')'))
                return node;
            expect(',');
        }
        error("too many argument: %s", t->input);
    }
    error("expect number or (, defined variable: %s", t->input);
    return node;
}

static Node *mul() {
    Node *lhs = term();
    for (;;) {
        Token *t = peek();
        if (t->ty != '*' && t->ty != '/')
            break;
        pos++;
        lhs = new_binop(t->ty, lhs, term());
    }
    return lhs;
}

static Node *add() {
    Node *lhs = mul();
    for (;;) {
        Token *t = peek();
        if (t->ty != '+' && t->ty != '-')
            break;
        pos++;
        lhs = new_binop(t->ty, lhs, mul());
    }
    return lhs;
}

static Node *equality() {
    Node *lhs = add();
    for (;;) {
        if (consume(TK_EQ)) {
            lhs = new_binop(ND_EQ, lhs, add());
            continue;
        }
        if (consume(TK_NE)) {
            lhs = new_binop(ND_NE, lhs, add());
            continue;
        }
        break;
    }
    return lhs;
}

static Node *logand() {
    Node *lhs = equality();
    for (;;) {
        if (consume(TK_LOGAND)) {
            lhs = new_binop(ND_LOGAND, lhs, equality());
            continue;
        }
        break;
    }
    return lhs;
}

static Node *logor() {
    Node *lhs = logand();
    for (;;) {
        if (consume(TK_LOGOR)) {
            lhs = new_binop(ND_LOGOR, lhs, logand());
            continue;
        }
        break;
    }
    return lhs;
}

static Node *assign() {
    Node *lhs = logor();
    if (consume('=')) {
        return new_binop('=', lhs, assign());
    }
    return lhs;
}

static Node *expr_stmt() {
    Node *node = new_node();
    node->op = ND_EXPR_STMT;
    node->expr = assign();
    expect(';');
    return node;
}

static Node *decl() {
    expect(TK_INT);
    Token *t = next();
    if (t->ty != TK_IDENT)
        error("not define variable: %s", t->input);
    Node *node = new_node();
    node->op = ND_VARDEF;
    node->name = t->name;

    if (consume('='))
        node->expr = assign();
    else
        node->expr = NULL;
    expect(';');
    return node;
}

static Node *stmt() {
    Token *t = peek();
    Node *node;
    switch (t->ty) {
    case TK_IF:
        pos++;
        node = new_node();
        node->op = ND_IF;
        expect('(');
        node->cond = assign();
        expect(')');
        node->then = stmt();
        if (consume(TK_ELSE)) {
            node->els = stmt();
        }
        return node;
    case TK_RETURN:
        pos++;
        node = new_node();
        node->op = ND_RETURN;
        node->expr = assign();
        expect(';');
        return node;
    case TK_INT:
        return decl();
    case TK_FOR:
        pos++;
        node = new_node();
        node->op = ND_FOR;
        expect('(');
        if (is_typename())
            node->init = decl();
        else
            node->init = expr_stmt();
        node->cond = assign();
        expect(';');
        node->incr = assign();
        expect(')');
        node->body = stmt();
        return node;
    case '{':
        pos++;
        node = compound_stmt();
        return node;
    default:
        return expr_stmt();
    }
}

static Node *compound_stmt() {
    Node *node = new_node();
    node->op = ND_COMP_STMT;
    node->stmts = new_vector();
    while (!consume('}')) {
        vec_push(node->stmts, stmt());
    }
    return node;
}

static Vector *argsdef() {
    Vector *args = new_vector();
    if (consume(')'))
        return args;
    expect(TK_INT);
    Token *t = next();
    if (t->ty != TK_IDENT)
        error("function argument must be literal: %s", t->input);
    vec_push(args, t->name);
    for (int i = 0; i < 6; i++) {
        if (consume(')'))
            return args;
        expect(',');
        expect(TK_INT);
        t = next();
        if (t->ty != TK_IDENT)
            error("function argument must be literal: %s", t->input);
        vec_push(args, t->name);
    }
    error("too many argument: %s", t->input);
    return NULL;
}

static Node *toplevel() {
    expect(TK_INT);
    Token *t = next();
    if (t->ty != TK_IDENT)
        error("must be function definition or global variable: %s", t->input);
    Node *node = new_node();
    node->name = t->name;
    if (consume('(')) {
        // Function definition
        node->op = ND_FUNC;
        node->args = argsdef();
        expect('{');
        node->body = compound_stmt();
        return node;
    }

    // Global Variable
    node->op = ND_VARDEF;
    if (consume('=')) {
        t = next();
        if (t->ty != TK_NUM)
            error("global varialbe assignment only number: %s", t->input);
        node->val = t->val;
    } else {
        node->val = 0;
    }
    expect(';');
    return node;
}

Vector *parse(Vector *_tokens) {
    tokens = _tokens;
    Vector *nodes = new_vector();
    pos = 0;
    for (Token *t = tokens->data[pos]; t->ty != TK_EOF; t = tokens->data[pos]) {
        vec_push(nodes, toplevel());
    }
    return nodes;
}
