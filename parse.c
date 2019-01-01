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
    Token *t = tokens->data[pos++];
    if (t->ty != ty) {
        error("expected %c", ty);
    }
}

static Node *assign();

static Node *term() {
    Token *t = tokens->data[pos++];
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
    error("expect number or (: %s", t->input);
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
    if (consume(TK_IF)) {
        Node *node = new_node();
        node->ty = ND_IF;
        expect('(');
        node->cond = assign();
        expect(')');
        node->then = stmt();
        if (consume(TK_ELSE)) {
            node->els = stmt();
        }
        return node;
    }
    if (consume(TK_RETURN)) {
        Node *node = new_node();
        node->ty = ND_RETURN;
        node->expr = assign();
        expect(';');
        return node;
    }
    Node *node = assign();
    if (!consume(';')) {
        Token *t = tokens->data[pos];
        error("expect ;: %s\n", t->input);
    }
    return node;
}

static Node *function() {
    Token *t = tokens->data[pos++];
    if (t->ty != TK_IDENT)
        error("must be function definition: %s", t->input);
    Node *node = new_node();
    node->ty = ND_FUNC;
    node->name = t->name;
    node->body = new_vector();
    expect('(');
    // TODO: multi args
    expect(')');
    expect('{');
    while (!consume('}')) {
        vec_push(node->body, stmt());
    }
    return node;
}

Vector *parse(Vector *_tokens) {
    tokens = _tokens;
    codes = new_vector();
    pos = 0;
    for (Token *t = tokens->data[pos]; t->ty != TK_EOF; t = tokens->data[pos]) {
        vec_push(codes, function());
    }
    return codes;
}
