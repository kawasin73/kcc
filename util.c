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

void vec_pushi(Vector *vec, int elem) {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wint-conversion"
    #pragma GCC diagnostic ignored "-Wint-to-void-pointer-cast"
    vec_push(vec, elem);
    #pragma GCC diagnostic pop
}

int vec_geti(Vector *vec, int idx) {
    return (int)vec->data[idx];
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

StringBuilder *new_sb() {
    StringBuilder *sb = malloc(sizeof(StringBuilder));
    sb->buf = malloc(sizeof(char) * 16);
    sb->len = 0;
    sb->capacity = 16;
    return sb;
}

void sb_add(StringBuilder *sb, char c) {
    if (sb->len+1 == sb->capacity) {
        sb->capacity *= 2;
        sb->buf = realloc(sb->buf, sizeof(char) * sb->capacity);
    }
    sb->buf[sb->len++] = c;
}

char *sb_string(StringBuilder *sb) {
    sb->buf[sb->len] = '\0';
    return sb->buf;
}

Type *new_type(int ty) {
    Type *t = calloc(1, sizeof(Type));
    t->ty = ty;
    return t;
}

int is_ptr(Type *ty) {
    return ty->ty == PTR || ty->ty == ARY;
}

Type *ptr_of(Type *ty) {
    Type *t = new_type(PTR);
    t->ptr_of = ty;
    return t;
}

Type *ary_of(Type *ty, int len) {
    Type *t = new_type(ARY);
    t->ptr_of = ty;
    t->len = len;
    return t;
}

int dig_ptr_of(Type *ty) {
    if (is_ptr(ty))
        return dig_ptr_of(ty->ptr_of);
    return ty->ty;
}

int register_size(Type *ty) {
    switch (ty->ty) {
    case PTR:
    case ARY:
    case INT:
        return 8;
    case CHAR:
        return 1;
    default:
        assert(0 && "invalid Type");
    }
}

int size_of(Type *ty) {
    switch (ty->ty) {
    case ARY:
        return size_of(ty->ptr_of) * ty->len;
    default:
        return register_size(ty);
    }
}

int equal_ty(Type *a, Type *b) {
    if (a->ty == PTR && b->ty == PTR)
        return equal_ty(a->ptr_of, b->ptr_of);
    if (a->ty == ARY && b->ty == ARY)
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

void debug(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
}

char *format(char *fmt, ...) {
    char buf[2048];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    return strdup(buf);
}
