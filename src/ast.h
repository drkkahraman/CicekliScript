#ifndef METESCRIPT_AST_H
#define METESCRIPT_AST_H

#include "lexer.h"
#include "value.h"

typedef enum {
    EXPR_LITERAL,
    EXPR_IDENTIFIER,
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_CALL,
    EXPR_MEMBER,
    EXPR_INDEX,
    EXPR_LIST,
    EXPR_MAP,
    EXPR_ASSIGN,
    EXPR_MEMBER_ASSIGN,
    EXPR_INDEX_ASSIGN,
    EXPR_FUNCTION
} ExprType;

typedef struct Expr Expr;
typedef struct Stmt Stmt;

typedef struct {
    char* key;
    Expr* value;
} AstMapEntry;

struct Expr {
    ExprType type;
    int line;
    struct Expr* next_allocated;
    union {
        Value literal;
        char* identifier;
        struct {
            Expr* left;
            TokenType op;
            Expr* right;
        } binary;
        struct {
            TokenType op;
            Expr* right;
        } unary;
        struct {
            Expr* callee;
            Expr** args;
            size_t arg_count;
        } call;
        struct {
            Expr* object;
            char* member;
        } member;
        struct {
            Expr* object;
            Expr* index;
        } index;
        struct {
            Expr** elements;
            size_t count;
        } list;
        struct {
            AstMapEntry* entries;
            size_t count;
        } map;
        struct {
            char* name;
            TokenType op;
            Expr* value;
        } assign;
        struct {
            Expr* object;
            char* member;
            Expr* value;
        } member_assign;
        struct {
            Expr* object;
            Expr* index;
            Expr* value;
        } index_assign;
        struct {
            char** params;
            size_t arity;
            Stmt* body;
        } function;
    } as;
};

typedef enum {
    STMT_EXPR,
    STMT_BLOCK,
    STMT_IF,
    STMT_WHILE,
    STMT_FOR_IN,
    STMT_FOR_C,
    STMT_FUNCTION,
    STMT_RETURN,
    STMT_BREAK,
    STMT_CONTINUE
} StmtType;

typedef struct {
    Expr* condition;
    Stmt* body;
} ElifClause;

struct Stmt {
    StmtType type;
    int line;
    struct Stmt* next_allocated;
    union {
        Expr* expr;
        struct {
            Stmt** stmts;
            size_t count;
        } block;
        struct {
            Expr* condition;
            Stmt* then_branch;
            ElifClause* elifs;
            size_t elif_count;
            Stmt* else_branch;
        } if_stmt;
        struct {
            Expr* condition;
            Stmt* body;
        } while_stmt;
        struct {
            char* var_name;
            Expr* iterable;
            Stmt* body;
        } for_in;
        struct {
            Stmt* init;
            Expr* cond;
            Expr* step;
            Stmt* body;
        } for_c;
        struct {
            char* name;
            char** params;
            size_t arity;
            Stmt* body;
        } function;
        Expr* return_expr;
    } as;
};

void ast_init(void);
void ast_free_all(void);

// Expression constructors
Expr* make_literal_expr(Value val, int line);
Expr* make_identifier_expr(const char* name, int line);
Expr* make_binary_expr(Expr* left, TokenType op, Expr* right, int line);
Expr* make_unary_expr(TokenType op, Expr* right, int line);
Expr* make_call_expr(Expr* callee, Expr** args, size_t arg_count, int line);
Expr* make_member_expr(Expr* object, const char* member, int line);
Expr* make_index_expr(Expr* object, Expr* index, int line);
Expr* make_list_expr(Expr** elements, size_t count, int line);
Expr* make_map_expr(AstMapEntry* entries, size_t count, int line);
Expr* make_assign_expr(const char* name, TokenType op, Expr* value, int line);
Expr* make_member_assign_expr(Expr* object, const char* member, Expr* value, int line);
Expr* make_index_assign_expr(Expr* object, Expr* index, Expr* value, int line);
Expr* make_function_expr(char** params, size_t arity, Stmt* body, int line);

// Statement constructors
Stmt* make_expr_stmt(Expr* expr, int line);
Stmt* make_block_stmt(Stmt** stmts, size_t count, int line);
Stmt* make_if_stmt(Expr* cond, Stmt* then_branch, ElifClause* elifs, size_t elif_count, Stmt* else_branch, int line);
Stmt* make_while_stmt(Expr* cond, Stmt* body, int line);
Stmt* make_for_in_stmt(const char* var, Expr* iter, Stmt* body, int line);
Stmt* make_for_c_stmt(Stmt* init, Expr* cond, Expr* step, Stmt* body, int line);
Stmt* make_function_stmt(const char* name, char** params, size_t arity, Stmt* body, int line);
Stmt* make_return_stmt(Expr* expr, int line);
Stmt* make_break_stmt(int line);
Stmt* make_continue_stmt(int line);

#endif // METESCRIPT_AST_H
