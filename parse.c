#include <stdlib.h>
#include "kcc.h"

static Vector *codes;
static Vector *tokens;
static int pos;

static Node *new_node() {
    return malloc(sizeof(Node));
}

static Node *new_binop(int ty, Node *lhs, Node *rhs) {
    Node *node = new_node();
    node->ty = ty;
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
    if (t->ty != ty) {
        error("expected %c (%d)", ty, ty);
    }
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
        node->ty = ND_NUM;
        node->val = t->val;
        return node;
    }
    if (t->ty == TK_IDENT) {
        node->name = t->name;
        if (!consume('(')) {
            node->ty = ND_IDENT;
            return node;
        }
        node->ty = ND_CALL;
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
    if (consume('*')) {
        return new_binop('*', lhs, mul());
    }
    if (consume('/')) {
        return new_binop('/', lhs, mul());
    }
    return lhs;
}

static Node *add() {
    Node *lhs = mul();
    if (consume('+')) {
        return new_binop('+', lhs, add());
    }
    if (consume('-')) {
        return new_binop('-', lhs, add());
    }
    return lhs;
}

static Node *equality() {
    Node *lhs = add();
    if (consume(TK_EQ)) {
        return new_binop(ND_EQ, lhs, add());
    }
    if (consume(TK_NE)) {
        return new_binop(ND_NE, lhs, add());
    }
    return lhs;
}

static Node *assign() {
    Node *lhs = equality();
    if (consume('=')) {
        return new_binop('=', lhs, assign());
    }
    return lhs;
}

static Node *stmt() {
    Token *t = peek();
    Node *node;
    switch (t->ty) {
    case TK_IF:
        pos++;
        node = new_node();
        node->ty = ND_IF;
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
        node->ty = ND_RETURN;
        node->expr = assign();
        expect(';');
        return node;
    case TK_INT:
        pos++;
        Token *t = next();
        if (t->ty != TK_IDENT) {
            error("not define variable: %s", t->input);
        }
        node = new_node();
        node->ty = ND_VARDEF;
        node->name = t->name;
        if (consume('=')) {
            node = new_binop('=', node, assign());
        }
        expect(';');
        return node;
    case '{':
        pos++;
        node = compound_stmt();
        return node;
    default:
        node = assign();
        expect(';');
        return node;
    }
}

static Node *compound_stmt() {
    Node *node = new_node();
    node->ty = ND_COMP_STMT;
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
        node->ty = ND_FUNC;
        node->args = argsdef();
        expect('{');
        node->body = compound_stmt();
        return node;
    }

    // Global Variable
    node->ty = ND_VARDEF;
    // TODO: initial data;
    expect(';');
    return node;
}

Vector *parse(Vector *_tokens) {
    tokens = _tokens;
    codes = new_vector();
    pos = 0;
    for (Token *t = tokens->data[pos]; t->ty != TK_EOF; t = tokens->data[pos]) {
        vec_push(codes, toplevel());
    }
    return codes;
}
