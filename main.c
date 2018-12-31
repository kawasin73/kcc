#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "kcc.h"

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
    TK_IDENT,
    TK_EQ,
    TK_NE,
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

        if ('a' <= *p && *p <= 'z') {
            tokens[i].ty = TK_IDENT;
            tokens[i].input = p;
            i++;
            p++;
            continue;
        }

        if (!strncmp(p, "==", 2)) {
            tokens[i].ty = TK_EQ;
            tokens[i].input = p;
            i++;
            p+=2;
            continue;
        }

        if (!strncmp(p, "!=", 2)) {
            tokens[i].ty = TK_NE;
            tokens[i].input = p;
            i++;
            p+=2;
            continue;
        }

        if (strchr("+-*/()=;", *p)) {
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
    ND_IDENT,
    ND_EQ,
    ND_NE,
};

int pos = 0;

typedef struct Node {
    int ty;
    struct Node *lhs;
    struct Node *rhs;
    int val;          // ty == ND_NUM
    char name;        // ty == ND_IDENT
} Node;

Node *code[100];
int cur = 0;

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

// Parser Structure
//
// program: assign program2
// program2: none | assign program2
// assign: equality assign2 ";"
// assign2: none | "=" equality assign2
// equality: expr
// equality: expr "==" expr
// equality: expr "!=" expr
// expr: mul
// expr: mul "+" expr
// expr: mul "-" expr
// mul: term
// mul: term "*" mul
// mul: term "/" mul
// term: number | ident
// term: "(" equality ")"
// number: digit | digit number
// digit: "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
// ident: "a" ~ "z"

Node *assign();
Node *equality();
Node *expr();
Node *mul();
Node *term();


void program() {
    Node *node = assign();
    if (!consume(';')) {
        error("expect ;: %s\n", tokens[pos].input);
    }
    code[cur] = node;
    cur++;
    if (tokens[pos].ty != TK_EOF) {
        program();
    }
    code[cur] = NULL;
}

Node *assign() {
    Node *lhs = equality();
    if (consume('=')) {
        return new_node('=', lhs, assign());
    }
    return lhs;
}

Node *equality() {
    Node *lhs = expr();
    if (consume(TK_EQ)) {
        return new_node(ND_EQ, lhs, expr());
    }
    if (consume(TK_NE)) {
        return new_node(ND_NE, lhs, expr());
    }
    return lhs;
}

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
    if (tokens[pos].ty == TK_IDENT)
        return new_node_ident(*tokens[pos++].input);
    if (!consume('('))
        error("expect number or (: %s", tokens[pos].input);
    Node *node = equality();
    if (!consume(')'))
        error("expect ): %s", tokens[pos].input);
    return node;
}

// ================================
// ASM Generator
// ================================

// push indicated address
void gen_lval(Node *node) {
    if (node->ty == ND_IDENT) {
        printf("  mov rax, rbp\n");
        printf("  sub rax, %d\n", ('z' - node->name + 1) * 8);
        printf("  push rax\n");
        return;
    }
    error("invalid value for assign");
}

void gen(Node *node) {
    if (node->ty == ND_NUM) {
        printf("  push %d\n", node->val);
        return;
    }

    if (node->ty == ND_IDENT) {
        gen_lval(node);
        printf("  pop rax\n");
        printf("  mov rax, [rax]\n");
        printf("  push rax\n");
        return;
    }

    if (node->ty == '=') {
        gen_lval(node->lhs);
        gen(node->rhs);
        printf("  pop rdi\n");
        printf("  pop rax\n");
        printf("  mov [rax], rdi\n");
        printf("  push rdi\n");
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
    case ND_EQ:
        printf("  cmp rdi, rax\n");
        printf("  sete al\n");
        printf("  movzx rax, al\n");
        break;
    case ND_NE:
        printf("  cmp rdi, rax\n");
        printf("  setne al\n");
        printf("  movzx rax, al\n");
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
    program();

    printf(".intel_syntax noprefix\n");
    printf(".global _main\n");
    printf("_main:\n");

    // prologue
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    // allocate stack frame for 26 * 8 bytes
    printf("  sub rsp, 208\n");

    for (int i = 0; code[i]; i++) {
        gen(code[i]);

        // last statement is return value
        printf("  pop rax\n");
    }

    // epilogue
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
    return 0;
}
