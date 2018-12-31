#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "kcc.h"

static Vector *tokens;

static Token *add_token(int ty, char *intput) {
    Token *token = malloc(sizeof(Token));
    token->ty = ty;
    token->input = intput;
    vec_push(tokens, token);
    return token;
}

// parse p to tokens
Vector *tokenize(char *p) {
    tokens = new_vector();
    while (*p) {
        // skip whitespace
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (isalpha(*p) || *p == '_') {
            int len = 1;
            while (isalpha(p[len]) || p[len] == '_' || isdigit(p[len])) {
                len++;
            }
            Token *t =add_token(TK_IDENT, p);
            t->name = strndup(p, len);
            p+=len;
            continue;
        }

        if (!strncmp(p, "==", 2)) {
            add_token(TK_EQ, p);
            p+=2;
            continue;
        }

        if (!strncmp(p, "!=", 2)) {
            add_token(TK_NE, p);
            p+=2;
            continue;
        }

        if (strchr("+-*/()=;", *p)) {
            add_token(*p, p);
            p++;
            continue;
        }

        if (isdigit(*p)) {
            Token *token = add_token(TK_NUM, p);
            token->val = strtol(p, &p, 10);
            continue;
        }

        error("can not tokenize: %s\n", p);
    }

    add_token(TK_EOF, p);
    return tokens;
}
