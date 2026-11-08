#ifndef METESCRIPT_EVAL_H
#define METESCRIPT_EVAL_H

#include "ast.h"
#include "env.h"
#include "value.h"

typedef enum {
    RES_OK,
    RES_RETURN,
    RES_BREAK,
    RES_CONTINUE,
    RES_ERROR
} EvalResultType;

typedef struct {
    EvalResultType type;
    Value value;
} EvalResult;

Environment* init_global_environment(void);
EvalResult eval_statement(Stmt* stmt, Environment* env);
EvalResult eval_statements(Stmt** stmts, size_t count, Environment* env);
Value eval_expression(Expr* expr, Environment* env, bool* had_error);

#endif // METESCRIPT_EVAL_H
