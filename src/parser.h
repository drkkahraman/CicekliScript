#ifndef CICEKLISCRIPT_PARSER_H
#define CICEKLISCRIPT_PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    Lexer lexer;
    Token current;
    Token previous;
    bool had_error;
    bool panic_mode;
} Parser;

void parser_init(Parser* parser, const char* source);
Stmt* parse_statement(Parser* parser);
Stmt** parse_program(Parser* parser, size_t* out_count);

#endif // CICEKLISCRIPT_PARSER_H
