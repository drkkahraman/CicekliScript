#include "ast.h"
#include <stdlib.h>
#include <string.h>

static Expr* all_exprs = NULL;
static Stmt* all_stmts = NULL;

void ast_init(void) {
    all_exprs = NULL;
    all_stmts = NULL;
}

static Expr* alloc_expr(ExprType type, int line) {
    Expr* e = (Expr*)malloc(sizeof(Expr));
    e->type = type;
    e->line = line;
    e->next_allocated = all_exprs;
    all_exprs = e;
    return e;
}

static Stmt* alloc_stmt(StmtType type, int line) {
    Stmt* s = (Stmt*)malloc(sizeof(Stmt));
    s->type = type;
    s->line = line;
    s->next_allocated = all_stmts;
    all_stmts = s;
    return s;
}

Expr* make_literal_expr(Value val, int line) {
    Expr* e = alloc_expr(EXPR_LITERAL, line);
    e->as.literal = val;
    return e;
}

Expr* make_identifier_expr(const char* name, int line) {
    Expr* e = alloc_expr(EXPR_IDENTIFIER, line);
    e->as.identifier = strdup(name);
    return e;
}

Expr* make_binary_expr(Expr* left, TokenType op, Expr* right, int line) {
    Expr* e = alloc_expr(EXPR_BINARY, line);
    e->as.binary.left = left;
    e->as.binary.op = op;
    e->as.binary.right = right;
    return e;
}

Expr* make_unary_expr(TokenType op, Expr* right, int line) {
    Expr* e = alloc_expr(EXPR_UNARY, line);
    e->as.unary.op = op;
    e->as.unary.right = right;
    return e;
}

Expr* make_call_expr(Expr* callee, Expr** args, size_t arg_count, int line) {
    Expr* e = alloc_expr(EXPR_CALL, line);
    e->as.call.callee = callee;
    e->as.call.args = args;
    e->as.call.arg_count = arg_count;
    return e;
}

Expr* make_member_expr(Expr* object, const char* member, int line) {
    Expr* e = alloc_expr(EXPR_MEMBER, line);
    e->as.member.object = object;
    e->as.member.member = strdup(member);
    return e;
}

Expr* make_index_expr(Expr* object, Expr* index, int line) {
    Expr* e = alloc_expr(EXPR_INDEX, line);
    e->as.index.object = object;
    e->as.index.index = index;
    return e;
}

Expr* make_list_expr(Expr** elements, size_t count, int line) {
    Expr* e = alloc_expr(EXPR_LIST, line);
    e->as.list.elements = elements;
    e->as.list.count = count;
    return e;
}

Expr* make_map_expr(AstMapEntry* entries, size_t count, int line) {
    Expr* e = alloc_expr(EXPR_MAP, line);
    e->as.map.entries = entries;
    e->as.map.count = count;
    return e;
}

Expr* make_assign_expr(const char* name, TokenType op, Expr* value, int line) {
    Expr* e = alloc_expr(EXPR_ASSIGN, line);
    e->as.assign.name = strdup(name);
    e->as.assign.op = op;
    e->as.assign.value = value;
    return e;
}

Expr* make_member_assign_expr(Expr* object, const char* member, Expr* value, int line) {
    Expr* e = alloc_expr(EXPR_MEMBER_ASSIGN, line);
    e->as.member_assign.object = object;
    e->as.member_assign.member = strdup(member);
    e->as.member_assign.value = value;
    return e;
}

Expr* make_index_assign_expr(Expr* object, Expr* index, Expr* value, int line) {
    Expr* e = alloc_expr(EXPR_INDEX_ASSIGN, line);
    e->as.index_assign.object = object;
    e->as.index_assign.index = index;
    e->as.index_assign.value = value;
    return e;
}

Expr* make_function_expr(char** params, size_t arity, Stmt* body, int line) {
    Expr* e = alloc_expr(EXPR_FUNCTION, line);
    e->as.function.params = params;
    e->as.function.arity = arity;
    e->as.function.body = body;
    return e;
}

Stmt* make_expr_stmt(Expr* expr, int line) {
    Stmt* s = alloc_stmt(STMT_EXPR, line);
    s->as.expr = expr;
    return s;
}

Stmt* make_block_stmt(Stmt** stmts, size_t count, int line) {
    Stmt* s = alloc_stmt(STMT_BLOCK, line);
    s->as.block.stmts = stmts;
    s->as.block.count = count;
    return s;
}

Stmt* make_if_stmt(Expr* cond, Stmt* then_branch, ElifClause* elifs, size_t elif_count, Stmt* else_branch, int line) {
    Stmt* s = alloc_stmt(STMT_IF, line);
    s->as.if_stmt.condition = cond;
    s->as.if_stmt.then_branch = then_branch;
    s->as.if_stmt.elifs = elifs;
    s->as.if_stmt.elif_count = elif_count;
    s->as.if_stmt.else_branch = else_branch;
    return s;
}

Stmt* make_while_stmt(Expr* cond, Stmt* body, int line) {
    Stmt* s = alloc_stmt(STMT_WHILE, line);
    s->as.while_stmt.condition = cond;
    s->as.while_stmt.body = body;
    return s;
}

Stmt* make_for_in_stmt(const char* var, Expr* iter, Stmt* body, int line) {
    Stmt* s = alloc_stmt(STMT_FOR_IN, line);
    s->as.for_in.var_name = strdup(var);
    s->as.for_in.iterable = iter;
    s->as.for_in.body = body;
    return s;
}

Stmt* make_for_c_stmt(Stmt* init, Expr* cond, Expr* step, Stmt* body, int line) {
    Stmt* s = alloc_stmt(STMT_FOR_C, line);
    s->as.for_c.init = init;
    s->as.for_c.cond = cond;
    s->as.for_c.step = step;
    s->as.for_c.body = body;
    return s;
}

Stmt* make_function_stmt(const char* name, char** params, size_t arity, Stmt* body, int line) {
    Stmt* s = alloc_stmt(STMT_FUNCTION, line);
    s->as.function.name = strdup(name);
    s->as.function.params = params;
    s->as.function.arity = arity;
    s->as.function.body = body;
    return s;
}

Stmt* make_return_stmt(Expr* expr, int line) {
    Stmt* s = alloc_stmt(STMT_RETURN, line);
    s->as.return_expr = expr;
    return s;
}

Stmt* make_break_stmt(int line) {
    return alloc_stmt(STMT_BREAK, line);
}

Stmt* make_continue_stmt(int line) {
    return alloc_stmt(STMT_CONTINUE, line);
}

void ast_free_all(void) {
    Expr* curr_e = all_exprs;
    while (curr_e) {
        Expr* next = curr_e->next_allocated;
        switch (curr_e->type) {
            case EXPR_IDENTIFIER:
                free(curr_e->as.identifier);
                break;
            case EXPR_CALL:
                free(curr_e->as.call.args);
                break;
            case EXPR_MEMBER:
                free(curr_e->as.member.member);
                break;
            case EXPR_LIST:
                free(curr_e->as.list.elements);
                break;
            case EXPR_MAP:
                for (size_t i = 0; i < curr_e->as.map.count; i++) {
                    free(curr_e->as.map.entries[i].key);
                }
                free(curr_e->as.map.entries);
                break;
            case EXPR_ASSIGN:
                free(curr_e->as.assign.name);
                break;
            case EXPR_MEMBER_ASSIGN:
                free(curr_e->as.member_assign.member);
                break;
            case EXPR_FUNCTION:
                for (size_t i = 0; i < curr_e->as.function.arity; i++) {
                    free(curr_e->as.function.params[i]);
                }
                free(curr_e->as.function.params);
                break;
            default:
                break;
        }
        free(curr_e);
        curr_e = next;
    }
    all_exprs = NULL;

    Stmt* curr_s = all_stmts;
    while (curr_s) {
        Stmt* next = curr_s->next_allocated;
        switch (curr_s->type) {
            case STMT_BLOCK:
                free(curr_s->as.block.stmts);
                break;
            case STMT_IF:
                free(curr_s->as.if_stmt.elifs);
                break;
            case STMT_FOR_IN:
                free(curr_s->as.for_in.var_name);
                break;
            case STMT_FUNCTION:
                free(curr_s->as.function.name);
                for (size_t i = 0; i < curr_s->as.function.arity; i++) {
                    free(curr_s->as.function.params[i]);
                }
                free(curr_s->as.function.params);
                break;
            default:
                break;
        }
        free(curr_s);
        curr_s = next;
    }
    all_stmts = NULL;
}
