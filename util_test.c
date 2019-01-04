#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kcc.h"

void expect(int line, int expected, int actual) {
    if (expected == actual)
        return;
    error("%d: %d expected, but got %d\n", line, expected, actual);
}

void expect_string(int line, char *expected, char *actual) {
    if (strcmp(expected, actual) == 0)
        return;
    error("%d: \"%s\" expected, but got \"%s\"\n", line, expected, actual);
}

void test_vector() {
    Vector *vec = new_vector();
    expect(__LINE__, 0, vec->len);

    for (int i = 0; i < 100; i++) {
        vec_pushi(vec, i);
    }

    expect(__LINE__, 100, vec->len);
    expect(__LINE__, 0, (int)vec->data[0]);
    expect(__LINE__, 50, (int)vec->data[50]);
    expect(__LINE__, 99, (int)vec->data[99]);
}

void test_map() {
    Map *map = new_map();
    expect(__LINE__, 0, (int)map_get(map, "foo"));
    expect(__LINE__, 0, (int)map_exists(map, "foo"));

    map_put(map, "foo", (void *)2);
    expect(__LINE__, 2, (int)map_get(map, "foo"));
    expect(__LINE__, 1, (int)map_exists(map, "foo"));

    map_put(map, "bar", (void *)4);
    expect(__LINE__, 2, (int)map_get(map, "foo"));
    expect(__LINE__, 4, (int)map_get(map, "bar"));

    map_put(map, "foo", (void *)6);
    expect(__LINE__, 6, (int)map_get(map, "foo"));

    map_puti(map, "baz", 8);
    expect(__LINE__, 8, map_geti(map, "baz"));
}

void test_sb() {
    StringBuilder *sb = new_sb();

    expect_string(__LINE__, "", sb_string(sb));
    sb_add(sb, 'a');
    expect_string(__LINE__, "a", sb_string(sb));
    sb_add(sb, 'A');
    expect_string(__LINE__, "aA", sb_string(sb));
    sb_add(sb, '\n');
    expect_string(__LINE__, "aA\n", sb_string(sb));
}

void run_test() {
    test_vector();
    test_map();
    test_sb();

    printf("OK\n");
}
