#include <stdio.h>
#include "kcc.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "invalid argument length\n");
        return 1;
    }

    tokenize(argv[1]);
    parse();
    gen();
    return 0;
}
