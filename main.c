#include <stdio.h>
#include <string.h>
#include "kcc.h"

int main(int argc, char **argv) {
    if (argc == 2 && !strcmp(argv[1], "-test")) {
        run_test();
        return 0;
    }
    if (argc != 2) {
        fprintf(stderr, "invalid argument length\n");
        return 1;
    }

    Vector *tokens = tokenize(argv[1]);
    Vector *nodes = parse(tokens);
    Program *program = analyze(nodes);
    Vector *funcs = gen_ir(nodes);
    gen(program->globals, program->strs, funcs);
    return 0;
}
