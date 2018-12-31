#include <stdio.h>
#include <stdlib.h>
#include "kcc.h"

void expect(int line, int expected, int actual) {
    if (expected == actual)
        return;
    error("%d: %d expected, but got %d\n", line, expected, actual);
}

void test_vector() {
    Vector *vec = new_vector();
    expect(__LINE__, 0, vec->len);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
    for (int i = 0; i < 100; i++) {
        vec_push(vec, (void *)i);
    }
#pragma GCC diagnostic pop

    expect(__LINE__, 100, vec->len);
    expect(__LINE__, 0, (int)vec->data[0]);
    expect(__LINE__, 50, (int)vec->data[50]);
    expect(__LINE__, 99, (int)vec->data[99]);
}

void run_test() {
    test_vector();
    printf("OK\n");
}
