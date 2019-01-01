// ================================
// util.c
// ================================

typedef struct {
    void **data;
    int capacity;
    int len;
} Vector;

typedef struct {
    Vector *keys;
    Vector *vals;
} Map;

Vector *new_vector();
void vec_push(Vector *vec, void *elem);

Map *new_map();
void map_put(Map *map, char *key, void *val);
int map_exists(Map *map, char *key);
void *map_get(Map *map, char *key);

void error(char *fmt, ...);

// ================================
// util_test.c
// ================================

void run_test();

// ================================
// token.c
// ================================

// token type
enum {
    TK_NUM = 256,
    TK_IDENT,
    TK_EQ,
    TK_NE,
    TK_IF,
    TK_EOF,
};

typedef struct {
    int ty;      // token type
    int val;     // number value for TK_NUM
    char *name;  // name for TK_IDENT
    char *input; // original token (for error message)
} Token;

Vector *tokenize(char *p);

// ================================
// parse.c
// ================================

enum {
    ND_NUM = 256,
    ND_IDENT,
    ND_EQ,
    ND_NE,
    ND_IF,
};

typedef struct Node {
    int ty;
    struct Node *lhs;
    struct Node *rhs;
    int val;          // ty == ND_NUM
    char *name;       // ty == ND_IDENT
} Node;

Vector *parse(Vector *tokens);

// ================================
// ir.c
// ================================

enum {
    IR_PUSH_IMM = 256,
    IR_PUSH_VAR_PTR,
    IR_POP,
    IR_LOAD_VAL,
    IR_ASSIGN,
    IR_EQ,
    IR_NE,
    IR_LABEL,
    IR_UNLESS,
};

typedef struct {
    int ty;
    int val;
} IR;

typedef struct {
    Vector *codes;
    int varsiz;
} Program;

Program *gen_ir(Vector *nodes);

// ================================
// gen.c
// ================================

void gen(Program *program);
