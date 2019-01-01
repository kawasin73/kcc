#include <stdlib.h>
#include "kcc.h"

static Vector *codes;
static Vector *tokens;
static int pos;

static Node *new_node(int ty, Node *lhs, Node *rhs) {
    Node *node = malloc(sizeof(Node));
    node->ty = ty;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_node_num(int val) {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_NUM;
    node->val = val;
    return node;
}

static Node *new_node_ident(char *name) {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_IDENT;
    node->name = name;
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

static Node *primary() {
    Token *t = tokens->data[pos++];
    if (t->ty == TK_NUM)
        return new_node_num(t->val);
    if (t->ty == TK_IDENT)
        return new_node_ident(t->name);
    if (t->ty != '(')
        error("expect number or (: %s", t->input);
    Node *node = assign();
    expect(')');
    return node;
}

static Node *mul() {
    Node *lhs = primary();
    if (consume('*')) {
        return new_node('*', lhs, mul());
    }
    if (consume('/')) {
        return new_node('/', lhs, mul());
    }
    return lhs;
}

static Node *add() {
    Node *lhs = mul();
    if (consume('+')) {
        return new_node('+', lhs, add());
    }
    if (consume('-')) {
        return new_node('-', lhs, add());
    }
    return lhs;
}

static Node *equality() {
    Node *lhs = add();
    if (consume(TK_EQ)) {
        return new_node(ND_EQ, lhs, add());
    }
    if (consume(TK_NE)) {
        return new_node(ND_NE, lhs, add());
    }
    return lhs;
}

static Node *assign() {
    Node *lhs = equality();
    if (consume('=')) {
        return new_node('=', lhs, assign());
    }
    return lhs;
}

static Node *stmt() {
    if (consume(TK_IF)) {
        expect('(');
        Node *cond = assign();
        expect(')');
        Node *body = stmt();
        return new_node(ND_IF, cond, body);
    }
    Node *node = assign();
    if (!consume(';')) {
        Token *t = tokens->data[pos];
        error("expect ;: %s\n", t->input);
    }
    return node;
}

Vector *parse(Vector *_tokens) {
    tokens = _tokens;
    codes = new_vector();
    pos = 0;
    for (Token *t = tokens->data[pos]; t->ty != TK_EOF; t = tokens->data[pos]) {
        vec_push(codes, stmt());
    }
    vec_push(codes, NULL);
    return codes;
}
