// ================================
// util.c
// ================================

typedef struct {
    void **data;
    int capacity;
    int len;
} Vector;

Vector *new_vector();
void vec_push(Vector *vec, void *elem);
void vec_pushi(Vector *vec, int elem);
int vec_geti(Vector *vec, int idx);

typedef struct {
    Vector *keys;
    Vector *vals;
} Map;

Map *new_map();
void map_put(Map *map, char *key, void *val);
void map_puti(Map *map, char *key, int val);
int map_exists(Map *map, char *key);
void *map_get(Map *map, char *key);
int map_geti(Map *map, char *key);

typedef struct {
    char *buf;
    int capacity;
    int len;
} StringBuilder;

StringBuilder *new_sb();
void sb_add(StringBuilder *sb, char c);
char *sb_string(StringBuilder *sb);

enum {
    INT,  // "int"
    CHAR, // "char"
    PTR,  // pointer
    ARY,  // array
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
int dig_ptr_of(Type *ty);
int register_size(Type *ty);
int size_of(Type *ty);
int equal_ty(Type *a, Type *b);

char *format(char *fmt, ...);;
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
    TK_STR,       // String literal
    TK_IDENT,     // Identifier
    TK_EQ,        // "=="
    TK_NE,        // "!="
    TK_LOGAND,    // "&&"
    TK_LOGOR,     // "||"
    TK_SIZEOF,    // "sizeof"
    TK_EXTERN,    // "extern"
    TK_INT,       // "int"
    TK_CHAR,      // "char"
    TK_IF,        // "if"
    TK_ELSE,      // "else"
    TK_FOR,       // "for"
    TK_DO,        // "do"
    TK_WHILE,     // "while"
    TK_RETURN,    // "return"
    TK_EOF,       // end of file
};

typedef struct {
    int ty;      // token type
    int val;     // number value for TK_NUM
    char *name;  // name for TK_IDENT
    char *data;  // data for TK_STR
    char *input; // original token (for error message)
} Token;

Vector *tokenize(char *p);

// ================================
// parse.c
// ================================

enum {
    ND_NUM = 256, // Number literal
    ND_STR,       // String literal
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
    ND_DO_WHILE,  // "do" ~ "while"
    ND_CALL,      // Function call
    ND_FUNCDEF,   // Function pre definition
    ND_FUNC,      // Function definition
    ND_RETURN,    // "return"
    ND_COMP_STMT, // Compound statements
    ND_EXPR_STMT, // Expression statement
    ND_STMT_EXPR, // Statement expression (GNU extention) `int a = {return 0;};`
};

typedef struct Node {
    int op;           // node operation
    struct Node *lhs;
    struct Node *rhs;

    int is_extern;  // "extern"

    // ty == ND_NUM
    int val;
    // ty == ND_STR
    char *str;

    // endlabel for "return"
    int endlabel;

    // Function name or Identifier or String literal label
    char *name;
    // Identifier type
    Type *ty;
    // local variable offset. global variable is -1.
    int offset;

    // "if" ( cond ) then "else" els
    // "for" ( init ; cond ; incr ) body
    // "do" then "while" ( cond )
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
// analyze.c
// ================================

typedef struct {
    Vector *globals;
    Vector *strs;
} Program;

typedef struct {
    Type *ty;
    int val;
    char *str;
} Literal;

typedef struct {
    char *name;
    Type *ty;
    int siz;
    int offset;
    int is_extern;
    Literal *initial;
} Var;

// returns global vars
Program *analyze(Vector *nodes);

// ================================
// ir.c
// ================================

enum {
    IR_PUSH_IMM = 256,
    IR_PUSH_VAR_PTR,
    IR_PUSH,
    IR_POP,
    IR_ADDR_LABEL,
    IR_ADDR_GLOBAL,
    IR_LOAD_VAL,
    IR_ASSIGN,
    IR_SET_ARG,
    IR_EQ,
    IR_NE,
    IR_LABEL,
    IR_LABEL_END,
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
    int endlabel;
} Function;

Vector *gen_ir(Vector *nodes);

// ================================
// gen.c
// ================================

void gen(Vector *globals, Vector *strs, Vector *funcs);
