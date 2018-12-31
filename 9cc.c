#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>


void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// ================================
// Tokenizer
// ================================

// token type
enum {
    TK_NUM = 256,
    TK_EOF,
};

typedef struct {
    int ty;      // token type
    int val;     // number value for TK_NUM
    char *input; // original token (for error message)
} Token;

Token tokens[100];

// parse p to tokens
void tokenize(char *p) {
    int i = 0;
    while (*p) {
        // skip whitespace
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')') {
            tokens[i].ty = *p;
            tokens[i].input = p;
            i++;
            p++;
            continue;
        }

        if (isdigit(*p)) {
            tokens[i].ty = TK_NUM;
            tokens[i].input = p;
            tokens[i].val = strtol(p, &p, 10);
            i++;
            continue;
        }

        fprintf(stderr, "can not tokenize: %s\n", p);
        exit(1);
    }

    tokens[i].ty = TK_EOF;
    tokens[i].input = p;
}

// ================================
// AST Parser
// ================================

enum {
    ND_NUM = 256,
};

int pos = 0;

typedef struct Node {
    int ty;
    struct Node *lhs;
    struct Node *rhs;
    int val;
} Node;

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

// returns true (1) or false (0)
int consume(int c) {
    if (tokens[pos].ty != c) {
        return 0;
    }
    pos++;
    return 1;
}

// Parser Structure
//
// expr: mul
// expr: mul "+" expr
// expr: mul "-" expr
// mul: term
// mul: term "*" mul
// mul: term "/" mul
// term: number
// term: "(" expr ")"
// number: digit | digit number
// digit: "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"

Node *mul();
Node *term();

Node *expr() {
    Node *lhs = mul();
    if (consume('+')) {
        return new_node('+', lhs, expr());
    }
    if (consume('-')) {
        return new_node('-', lhs, expr());
    }
    return lhs;
}

Node *mul() {
    Node *lhs = term();
    if (consume('*')) {
        return new_node('*', lhs, mul());
    }
    if (consume('/')) {
        return new_node('/', lhs, mul());
    }
    return lhs;
}

Node *term() {
    if (tokens[pos].ty == TK_NUM)
        return new_node_num(tokens[pos++].val);
    if (!consume('('))
        error("expect number or (: %s", tokens[pos].input);
    Node *node = expr();
    if (!consume(')'))
        error("expect ): %s", tokens[pos].input);
    return node;
}

// ================================
// ASM Generator
// ================================

void gen(Node *node) {
    if (node->ty == ND_NUM) {
        printf("  push %d\n", node->val);
        return;
    }
    gen(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch (node->ty) {
    case '+':
        printf("  add rax, rdi\n");
        break;
    case '-':
        printf("  sub rax, rdi\n");
        break;
    case '*':
        printf("  mul rdi\n");
        break;
    case '/':
        printf("  mov rdx, 0\n");
        printf("  div rdi\n");
        break;
    }

    printf("  push rax\n");
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "invalid argument length\n");
        return 1;
    }

    tokenize(argv[1]);
    Node *node = expr();

    printf(".intel_syntax noprefix\n");
    printf(".global _main\n");
    printf("_main:\n");

    gen(node);

    printf("  pop rax\n");
    printf("  ret\n");
    return 0;
}
