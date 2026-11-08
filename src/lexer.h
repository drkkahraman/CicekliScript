#ifndef METESCRIPT_LEXER_H
#define METESCRIPT_LEXER_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,     // ( )
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,     // { }
    TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET, // [ ]
    TOKEN_COMMA, TOKEN_DOT, TOKEN_COLON,     // , . :
    TOKEN_SEMICOLON,                         // ;

    TOKEN_PLUS, TOKEN_MINUS,                 // + -
    TOKEN_STAR, TOKEN_SLASH, TOKEN_PERCENT,  // * / %
    TOKEN_PLUS_PLUS, TOKEN_MINUS_MINUS,      // ++ --

    TOKEN_EQUAL,                             // =
    TOKEN_PLUS_EQUAL, TOKEN_MINUS_EQUAL,     // += -=
    TOKEN_STAR_EQUAL, TOKEN_SLASH_EQUAL,     // *= /=

    TOKEN_BANG, TOKEN_BANG_EQUAL,            // ! !=
    TOKEN_EQUAL_EQUAL,                       // ==
    TOKEN_GREATER, TOKEN_GREATER_EQUAL,      // > >=
    TOKEN_LESS, TOKEN_LESS_EQUAL,            // < <=
    TOKEN_AMP_AMP, TOKEN_PIPE_PIPE,          // && ||

    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_NUMBER,

    TOKEN_METE_DEFFUNCTION,                  // mete.deffunction
    TOKEN_DEF,                               // def, deffunction
    TOKEN_IF, TOKEN_ELIF, TOKEN_ELSE,        // if, elif, else
    TOKEN_WHILE, TOKEN_FOR, TOKEN_IN,        // while, for, in
    TOKEN_RETURN, TOKEN_BREAK, TOKEN_CONTINUE,// return, break, continue
    TOKEN_TRUE, TOKEN_FALSE,                 // true, True, false, False
    TOKEN_NULL,                              // null, None, nil
    TOKEN_AND, TOKEN_OR, TOKEN_NOT,          // and, or, not

    TOKEN_ERROR,
    TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    size_t length;
    int line;
    double number_value;
    char* string_value;
} Token;

typedef struct {
    const char* source;
    const char* current;
    int line;
} Lexer;

void lexer_init(Lexer* lexer, const char* source);
Token lexer_next_token(Lexer* lexer);
const char* token_type_name(TokenType type);

#endif // METESCRIPT_LEXER_H
