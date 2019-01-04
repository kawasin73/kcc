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
void vec_pushi(Vector *vec, int elem);
int vec_geti(Vector *vec, int idx);

Map *new_map();
void map_put(Map *map, char *key, void *val);
void map_puti(Map *map, char *key, int val);
int map_exists(Map *map, char *key);
void *map_get(Map *map, char *key);
int map_geti(Map *map, char *key);

enum {
    INT, // "int"
    PTR, // pointer
    ARY, // array
};

typedef struct Type {
    int ty;
    // Pointer
    struct Type *ptr_of;
    // Array
    int len;
} Type;

Type *new_type(int ty);
int is_ptr(Type *ty);
Type *ptr_of(Type *ty);
Type *ary_of(Type *ty, int len);
int register_size(Type *ty);
int size_of(Type *ty);
int equal_ty(Type *a, Type *b);

void debug(char *fmt, ...);
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
    TK_NUM = 256, // Number literal
    TK_IDENT,     // Identifier
    TK_EQ,        // "=="
    TK_NE,        // "!="
    TK_LOGAND,    // "&&"
    TK_LOGOR,     // "||"
    TK_SIZEOF,    // "sizeof"
    TK_INT,       // "int"
    TK_IF,        // "if"
    TK_ELSE,      // "else"
    TK_FOR,       // "for"
    TK_RETURN,    // "return"
    TK_EOF,       // end of file
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
    ND_NUM = 256, // Number literal
    ND_VARDEF,    // Define variable
    ND_IDENT,     // Identifier
    ND_ADDR,      // "&" Reference
    ND_DEREF,     // "*" Dereference
    ND_SIZEOF,    // "sizeof"
    ND_EQ,        // "=="
    ND_NE,        // "!="
    ND_LOGAND,    // "&&"
    ND_LOGOR,     // "||"
    ND_IF,        // "if"
    ND_FOR,       // "for"
    ND_CALL,      // Function call
    ND_FUNC,      // Function definition
    ND_RETURN,    // "return"
    ND_COMP_STMT, // Compound statements
    ND_EXPR_STMT, // Expression statement
};

typedef struct {
    char *name;
    Type *ty;
    int siz;
    int offset;
    int initial;
} Var;

typedef struct Node {
    int op;           // node operation
    struct Node *lhs;
    struct Node *rhs;

    // ty == ND_NUM
    int val;

    // Function name or Identifier
    char *name;
    // Identifier type
    Type *ty;
    // local variable offset. global variable is -1.
    int offset;

    // "if" ( cond ) then "else" els
    // "for" ( init ; cond ; incr ) body
    struct Node *cond;
    struct Node *then;
    struct Node *els;
    struct Node *init;
    struct Node *incr;

    // Function call arguments
    Vector *args;

    // Function stack size
    int varsiz;
    // Function body
    struct Node *body;

    // Compound statements
    Vector *stmts;

    // "return"
    // Expression statement
    // inital value for ND_VARDEF
    // index for ND_DEREF
    struct Node *expr;
} Node;

Vector *parse(Vector *tokens);

// ================================
// ir.c
// ================================

// returns global vars
Vector *analyze(Vector *nodes);

// ================================
// ir.c
// ================================

enum {
    IR_PUSH_IMM = 256,
    IR_PUSH_VAR_PTR,
    IR_LABEL_ADDR,
    IR_POP,
    IR_LOAD_VAL,
    IR_ASSIGN,
    IR_SET_ARG,
    IR_EQ,
    IR_NE,
    IR_LABEL,
    IR_IF,
    IR_UNLESS,
    IR_LOGAND,
    IR_LOGOR,
    IR_JMP,
    IR_CALL,
    IR_RETURN,
};

typedef struct {
    int op;
    int val;
    int siz;
    char *name;
} IR;

typedef struct {
    char *name;
    Vector *codes;
    Vector *args;
    int varsiz;
} Function;

Vector *gen_ir(Vector *nodes);

// ================================
// gen.c
// ================================

void gen(Vector *globals, Vector *funcs);
