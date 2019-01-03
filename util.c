#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include "kcc.h"

Vector *new_vector() {
    Vector *vec = malloc(sizeof(Vector));
    vec->data = malloc(sizeof(void *) * 16);
    vec->capacity = 16;
    vec->len = 0;
    return vec;
}

void vec_push(Vector *vec, void *elem) {
    if (vec->capacity == vec->len) {
        vec->capacity *= 2;
        vec->data = realloc(vec->data, sizeof(void *) * vec->capacity);
    }
    vec->data[vec->len++] = elem;
}

Map *new_map() {
    Map *map = malloc(sizeof(Map));
    map->keys = new_vector();
    map->vals = new_vector();
    return map;
}

void map_put(Map *map, char *key, void *val) {
    vec_push(map->keys, key);
    vec_push(map->vals, val);
}

void map_puti(Map *map, char *key, int val) {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wint-conversion"
    #pragma GCC diagnostic ignored "-Wint-to-void-pointer-cast"
    map_put(map, key, (void *)val);
    #pragma GCC diagnostic pop
}

int map_exists(Map *map, char *key) {
    for (int i = 0; i < map->keys->len; i++) {
        if (strcmp(map->keys->data[i], key) == 0)
            return 1;
    }
    return 0;
}

void *map_get(Map *map, char *key) {
    for (int i = map->keys->len - 1; i >= 0; i--) {
        if (strcmp(map->keys->data[i], key) == 0) {
            return map->vals->data[i];
        }
    }
    return NULL;
}

int map_geti(Map *map, char *key) {
    return (int)map_get(map, key);
}

Type *new_type(int ty) {
    Type *t = calloc(1, sizeof(Type));
    t->ty = ty;
    return t;
}

int is_ptr(Type *ty) {
    return ty->ty == PTR || ty->ty == ARY;
}

int size_of(Type *ty) {
    switch (ty->ty) {
    case PTR:
    case ARY:
        return 8;
    case INT:
        return 8;
    default:
        assert(0 && "invalid Type");
    }
}

int alloc_size(Type *ty) {
    switch (ty->ty) {
    case INT:
        return 8;
    case PTR:
        return 8;
    case ARY:
        return alloc_size(ty->ptr_of) * ty->len;
    default:
        assert(0 && "invalid type");
    }
}

int equal_ty(Type *a, Type *b) {
    if (is_ptr(a) && is_ptr(b))
        return equal_ty(a->ptr_of, b->ptr_of);
    if (a->ty != b->ty)
        return 0;
    return 1;
}

void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}
