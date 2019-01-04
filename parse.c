#include <stdlib.h>
#include <string.h>
#include "kcc.h"

static Vector *tokens;
static int pos;
static Type int_ty = {INT};
static Type char_ty = {CHAR};

static Node *new_node() {
    return calloc(1, sizeof(Node));
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

static Type *type() {
    Token *t = next();
    switch (t->ty) {
    case TK_INT:
        return new_type(INT);
    case TK_CHAR:
        return new_type(CHAR);
    default:
        error("undefined type: %s", t->input);
        return NULL;
    }
}

static Node *expr();
static Node *compound_stmt();

static Node *term() {
    Token *t = next();
    Node *node;
    if (t->ty == '(') {
        node = expr();
        expect(')');
        return node;
    }
    node = new_node();
    if (t->ty == TK_NUM) {
        node->op = ND_NUM;
        node->val = t->val;
        node->ty = &int_ty;
        return node;
    }
    if (t->ty == TK_STR) {
        node->op = ND_STR;
        node->str = t->data;
        node->ty = ary_of(&char_ty, strlen(node->str)+1);
        return node;
    }
    if (t->ty == TK_IDENT) {
        node->name = t->name;
        if (!consume('(')) {
            // variable access
            node->op = ND_IDENT;
            return node;
        }

        // function call
        node->op = ND_CALL;
        node->args = new_vector();
        if (consume(')'))
            return node;
        for (int i = 0; i < 6; i++) {
            vec_push(node->args, expr());
            if (consume(')'))
                return node;
            expect(',');
        }
        error("too many argument: %s", t->input);
    }
    error("expect number or (, defined variable: %s", t->input);
    return node;
}

static Node *postfix() {
    Node *lhs = term();
    while (consume('[')) {
        Node *node = new_node();
        node->op = ND_DEREF;
        node->expr = new_binop('+', lhs, expr());
        lhs = node;
        expect(']');
    }
    return lhs;
}

static Node *unary() {
    Node *node;
    if (consume('*')) {
        node = new_node();
        node->op = ND_DEREF;
        node->expr = unary();
        return node;
    }
    if (consume('&')) {
        node = new_node();
        node->op = ND_ADDR;
        node->expr = unary();
        return node;
    }
    if (consume(TK_SIZEOF)) {
        node = new_node();
        node->op = ND_SIZEOF;
        node->expr = expr();
        return node;
    }
    return postfix();
}

static Node *mul() {
    Node *lhs = unary();
    for (;;) {
        Token *t = peek();
        if (t->ty != '*' && t->ty != '/')
            break;
        pos++;
        lhs = new_binop(t->ty, lhs, unary());
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
        return new_binop('=', lhs, expr());
    }
    return lhs;
}

static Node *stmt();

// alias
static Node *expr() {
    if (consume('{')) {
        Node *node = new_node();
        node->op = ND_STMT_EXPR;
        node->expr = compound_stmt();
        return node;
    }
    return assign();
}

static Node *expr_stmt() {
    Node *node = new_node();
    node->op = ND_EXPR_STMT;
    node->expr = expr();
    expect(';');
    return node;
}

static Type *array_param(Type *ty) {
    Vector *lens = new_vector();
    while (consume('[')) {
        Token *t = next();
        // TODO: calc on preprocess
        if (t->ty != TK_NUM)
            error("expect define array number: %s", t->input);
        expect(']');
        vec_pushi(lens, t->val);
    }

    // reverse Type tree
    for (int i = lens->len-1; i >= 0; i--) {
        ty = ary_of(ty, vec_geti(lens, i));
    }
    return ty;
}

static Node *param() {
    Type *ty = type();
    while (consume('*')) {
        ty = ptr_of(ty);
    }
    Token *t = next();
    if (t->ty != TK_IDENT)
        error("not define variable: %s", t->input);

    ty = array_param(ty);

    Node *node = new_node();
    node->op = ND_VARDEF;
    node->name = t->name;
    node->ty = ty;
    return node;
}

static Node *decl() {
    Node *node = param();
    if (consume('='))
        node->expr = expr();
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
        node->cond = expr();
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
        node->expr = expr();
        expect(';');
        return node;
    case TK_CHAR:
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
        node->cond = expr();
        expect(';');
        node->incr = expr();
        expect(')');
        node->body = stmt();
        return node;
    case TK_DO:
        pos++;
        node = new_node();
        node->op = ND_DO_WHILE;
        node->then = stmt();
        expect(TK_WHILE);
        expect('(');
        node->cond = expr();
        expect(')');
        expect(';');
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
    vec_push(args, param());
    for (int i = 0; i < 6; i++) {
        if (consume(')'))
            return args;
        expect(',');
        vec_push(args, param());
    }
    Token *t = tokens->data[pos];
    error("too many argument: %s", t->input);
    return NULL;
}

static Node *literal() {
    Token *t = next();
    Node *node = new_node();
    if (t->ty == TK_NUM) {
        node->op = ND_NUM;
        node->val = t->val;
        node->ty = &int_ty;
        return node;
    }
    if (t->ty == TK_STR) {
        node->op = ND_STR;
        node->str = t->data;
        node->ty = ary_of(&char_ty, strlen(node->str)+1);
        return node;
    }
    error("must be literal: %s", t->input);
    return NULL;
}

static Node *toplevel() {
    int is_extern = consume(TK_EXTERN);
    Node *node = param();
    if (consume('(')) {
        // Function definition
        if (node->ty->ty == ARY)
            error("function can not return array: %s", node->name);
        node->args = argsdef();
        if (consume(';')) {
            node->op = ND_FUNCDEF;
            return node;
        }
        node->op = ND_FUNC;
        expect('{');
        node->body = compound_stmt();
        return node;
    }

    // Global Variable
    node->is_extern = is_extern;
    if (!is_extern && consume('=')) {
        node->expr = literal();
    } else {
        node->expr = NULL;
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
