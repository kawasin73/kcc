#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "kcc.h"

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

        error("can not tokenize: %s\n", p);
    }

    tokens[i].ty = TK_EOF;
    tokens[i].input = p;
}
