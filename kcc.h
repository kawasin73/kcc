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
void map_puti(Map *map, char *key, int val);
int map_exists(Map *map, char *key);
void *map_get(Map *map, char *key);
int map_geti(Map *map, char *key);

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

typedef struct Node {
    int op;           // node operation
    struct Node *lhs;
    struct Node *rhs;

    // ty == ND_NUM
    int val;

    // Function name or Identifier
    char *name;

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
    struct Node *expr;

    // local variable offset
    int offset;
} Node;

Vector *parse(Vector *tokens);

// ================================
// ir.c
// ================================

typedef struct {
    int offset;
    char *name;
    int initial;
} Var;

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
    int ty;
    int val;
    char *name;
} IR;

typedef struct {
    char *name;
    Vector *codes;
    int args;
    int varsiz;
} Function;

Vector *gen_ir(Vector *nodes);

// ================================
// gen.c
// ================================

void gen(Vector *globals, Vector *funcs);
