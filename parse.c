#include <stdlib.h>
#include "kcc.h"

int pos = 0;
Node *code[100];

Node *new_node(int ty, Node *lhs, Node *rhs) {
    Node *node = malloc(sizeof(Node));
    node->ty = ty;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val) {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_NUM;
    node->val = val;
    return node;
}

Node *new_node_ident(char name) {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_IDENT;
    node->name = name;
    return node;
}

// returns true (1) or false (0)
int consume(int c) {
    if (tokens[pos].ty != c) {
        return 0;
    }
    pos++;
    return 1;
}

Node *assign();

Node *primary() {
    if (tokens[pos].ty == TK_NUM)
        return new_node_num(tokens[pos++].val);
    if (tokens[pos].ty == TK_IDENT)
        return new_node_ident(*tokens[pos++].input);
    if (!consume('('))
        error("expect number or (: %s", tokens[pos].input);
    Node *node = assign();
    if (!consume(')'))
        error("expect ): %s", tokens[pos].input);
    return node;
}

Node *mul() {
    Node *lhs = primary();
    if (consume('*')) {
        return new_node('*', lhs, mul());
    }
    if (consume('/')) {
        return new_node('/', lhs, mul());
    }
    return lhs;
}

Node *add() {
    Node *lhs = mul();
    if (consume('+')) {
        return new_node('+', lhs, add());
    }
    if (consume('-')) {
        return new_node('-', lhs, add());
    }
    return lhs;
}

Node *equality() {
    Node *lhs = add();
    if (consume(TK_EQ)) {
        return new_node(ND_EQ, lhs, add());
    }
    if (consume(TK_NE)) {
        return new_node(ND_NE, lhs, add());
    }
    return lhs;
}

Node *assign() {
    Node *lhs = equality();
    if (consume('=')) {
        return new_node('=', lhs, assign());
    }
    return lhs;
}

Node *stmt() {
    Node *node = assign();
    if (!consume(';')) {
        error("expect ;: %s\n", tokens[pos].input);
    }
    return node;
}

void parse() {
    int cur = 0;
    while (tokens[pos].ty != TK_EOF) {
        code[cur++] = stmt();
    }
    code[cur] = NULL;
}
