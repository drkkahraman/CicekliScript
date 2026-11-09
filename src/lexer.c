#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void lexer_init(Lexer* lexer, const char* source) {
    lexer->source = source;
    lexer->current = source;
    lexer->line = 1;
}

static bool is_at_end(Lexer* lexer) {
    return *lexer->current == '\0';
}

static char advance(Lexer* lexer) {
    return *lexer->current++;
}

static char peek(Lexer* lexer) {
    return *lexer->current;
}

static char peek_next(Lexer* lexer) {
    if (is_at_end(lexer)) return '\0';
    return lexer->current[1];
}

static bool match(Lexer* lexer, char expected) {
    if (is_at_end(lexer)) return false;
    if (*lexer->current != expected) return false;
    lexer->current++;
    return true;
}

static void skip_whitespace_and_comments(Lexer* lexer) {
    for (;;) {
        char c = peek(lexer);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance(lexer);
                break;
            case '\n':
                lexer->line++;
                advance(lexer);
                break;
            case '#': // Python comment
                while (peek(lexer) != '\n' && !is_at_end(lexer)) {
                    advance(lexer);
                }
                break;
            case '/':
                if (peek_next(lexer) == '/') { // C single-line comment
                    advance(lexer);
                    advance(lexer);
                    while (peek(lexer) != '\n' && !is_at_end(lexer)) {
                        advance(lexer);
                    }
                } else if (peek_next(lexer) == '*') { // C multi-line comment
                    advance(lexer);
                    advance(lexer);
                    while (!is_at_end(lexer)) {
                        if (peek(lexer) == '\n') lexer->line++;
                        if (peek(lexer) == '*' && peek_next(lexer) == '/') {
                            advance(lexer);
                            advance(lexer);
                            break;
                        }
                        advance(lexer);
                    }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static Token make_token(Lexer* lexer, TokenType type, const char* start) {
    Token token;
    token.type = type;
    token.start = start;
    token.length = (size_t)(lexer->current - start);
    token.line = lexer->line;
    token.number_value = 0.0;
    token.string_value = NULL;
    return token;
}

static Token error_token(Lexer* lexer, const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = strlen(message);
    token.line = lexer->line;
    token.number_value = 0.0;
    token.string_value = NULL;
    return token;
}

static Token scan_string(Lexer* lexer, char quote) {
    const char* start = lexer->current - 1;
    size_t cap = 32;
    size_t len = 0;
    char* buffer = (char*)malloc(cap);

    while (peek(lexer) != quote && !is_at_end(lexer)) {
        if (peek(lexer) == '\n') lexer->line++;
        char c = advance(lexer);
        if (c == '\\') {
            if (is_at_end(lexer)) break;
            char esc = advance(lexer);
            switch (esc) {
                case 'n': c = '\n'; break;
                case 't': c = '\t'; break;
                case 'r': c = '\r'; break;
                case '\\': c = '\\'; break;
                case '\"': c = '\"'; break;
                case '\'': c = '\''; break;
                case '0': c = '\0'; break;
                default: c = esc; break;
            }
        }
        if (len + 2 >= cap) {
            cap *= 2;
            buffer = (char*)realloc(buffer, cap);
        }
        buffer[len++] = c;
    }

    if (is_at_end(lexer)) {
        free(buffer);
        return error_token(lexer, "Kapatılmamış metin (string)!");
    }

    advance(lexer); // consume closing quote
    buffer[len] = '\0';

    Token token = make_token(lexer, TOKEN_STRING, start);
    token.string_value = buffer;
    return token;
}

static Token scan_number(Lexer* lexer, const char* start) {
    while (isdigit(peek(lexer))) {
        advance(lexer);
    }
    if (peek(lexer) == '.' && isdigit(peek_next(lexer))) {
        advance(lexer); // consume '.'
        while (isdigit(peek(lexer))) {
            advance(lexer);
        }
    }
    Token token = make_token(lexer, TOKEN_NUMBER, start);
    token.number_value = strtod(start, NULL);
    return token;
}

static TokenType check_keyword(const char* start, size_t length, const char* rest, size_t rest_len, TokenType type) {
    if (length == rest_len && memcmp(start, rest, rest_len) == 0) {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

static TokenType identifier_type(const char* start, size_t length) {
    switch (start[0]) {
        case 'a': return check_keyword(start, length, "and", 3, TOKEN_AND);
        case 'b': return check_keyword(start, length, "break", 5, TOKEN_BREAK);
        case 'c': return check_keyword(start, length, "continue", 8, TOKEN_CONTINUE);
        case 'd':
            if (length == 3 && memcmp(start, "def", 3) == 0) return TOKEN_DEF;
            if (length == 11 && memcmp(start, "deffunction", 11) == 0) return TOKEN_DEF;
            break;
        case 'e':
            if (length == 4 && memcmp(start, "elif", 4) == 0) return TOKEN_ELIF;
            if (length == 4 && memcmp(start, "else", 4) == 0) return TOKEN_ELSE;
            break;
        case 'f':
            if (length == 3 && memcmp(start, "for", 3) == 0) return TOKEN_FOR;
            if (length == 5 && memcmp(start, "false", 5) == 0) return TOKEN_FALSE;
            break;
        case 'F':
            if (length == 5 && memcmp(start, "False", 5) == 0) return TOKEN_FALSE;
            break;
        case 'i':
            if (length == 2 && memcmp(start, "if", 2) == 0) return TOKEN_IF;
            if (length == 2 && memcmp(start, "in", 2) == 0) return TOKEN_IN;
            if (length == 6 && memcmp(start, "import", 6) == 0) return TOKEN_IMPORT;
            break;
        case 'n':
            if (length == 4 && memcmp(start, "null", 4) == 0) return TOKEN_NULL;
            if (length == 3 && memcmp(start, "not", 3) == 0) return TOKEN_NOT;
            if (length == 3 && memcmp(start, "nil", 3) == 0) return TOKEN_NULL;
            break;
        case 'N':
            if (length == 4 && memcmp(start, "None", 4) == 0) return TOKEN_NULL;
            break;
        case 'o': return check_keyword(start, length, "or", 2, TOKEN_OR);
        case 'r': return check_keyword(start, length, "return", 6, TOKEN_RETURN);
        case 't': return check_keyword(start, length, "true", 4, TOKEN_TRUE);
        case 'T': return check_keyword(start, length, "True", 4, TOKEN_TRUE);
        case 'w': return check_keyword(start, length, "while", 5, TOKEN_WHILE);
    }
    return TOKEN_IDENTIFIER;
}

static Token scan_identifier_or_keyword(Lexer* lexer, const char* start) {
    // Check for cicekli.deffunction, cicekliscript.deffunction, mete.deffunction
    if (strncmp(start, "cicekli.deffunction", 19) == 0 &&
        !isalnum(start[19]) && start[19] != '_') {
        lexer->current = start + 19;
        return make_token(lexer, TOKEN_CICEKLI_DEFFUNCTION, start);
    }
    if (strncmp(start, "cicekliscript.deffunction", 25) == 0 &&
        !isalnum(start[25]) && start[25] != '_') {
        lexer->current = start + 25;
        return make_token(lexer, TOKEN_CICEKLI_DEFFUNCTION, start);
    }
    if (strncmp(start, "mete.deffunction", 16) == 0 &&
        !isalnum(start[16]) && start[16] != '_') {
        lexer->current = start + 16;
        return make_token(lexer, TOKEN_CICEKLI_DEFFUNCTION, start);
    }

    while (isalnum(peek(lexer)) || peek(lexer) == '_') {
        advance(lexer);
    }

    // If identifier is "cicekli", "cicekliscript" or "mete" and next is ".deffunction"
    size_t len = (size_t)(lexer->current - start);
    if (((len == 7 && memcmp(start, "cicekli", 7) == 0) ||
         (len == 13 && memcmp(start, "cicekliscript", 13) == 0) ||
         (len == 4 && memcmp(start, "mete", 4) == 0)) && peek(lexer) == '.') {
        if (strncmp(lexer->current, ".deffunction", 12) == 0 &&
            !isalnum(lexer->current[12]) && lexer->current[12] != '_') {
            lexer->current += 12;
            return make_token(lexer, TOKEN_CICEKLI_DEFFUNCTION, start);
        }
    }

    TokenType type = identifier_type(start, len);
    return make_token(lexer, type, start);
}

Token lexer_next_token(Lexer* lexer) {
    skip_whitespace_and_comments(lexer);

    if (is_at_end(lexer)) {
        return make_token(lexer, TOKEN_EOF, lexer->current);
    }

    const char* start = lexer->current;
    char c = advance(lexer);

    if (isalpha(c) || c == '_') {
        return scan_identifier_or_keyword(lexer, start);
    }

    if (isdigit(c)) {
        return scan_number(lexer, start);
    }

    switch (c) {
        case '(': return make_token(lexer, TOKEN_LEFT_PAREN, start);
        case ')': return make_token(lexer, TOKEN_RIGHT_PAREN, start);
        case '{': return make_token(lexer, TOKEN_LEFT_BRACE, start);
        case '}': return make_token(lexer, TOKEN_RIGHT_BRACE, start);
        case '[': return make_token(lexer, TOKEN_LEFT_BRACKET, start);
        case ']': return make_token(lexer, TOKEN_RIGHT_BRACKET, start);
        case ',': return make_token(lexer, TOKEN_COMMA, start);
        case '.': return make_token(lexer, TOKEN_DOT, start);
        case ':': return make_token(lexer, TOKEN_COLON, start);
        case ';': return make_token(lexer, TOKEN_SEMICOLON, start);

        case '+':
            if (match(lexer, '+')) return make_token(lexer, TOKEN_PLUS_PLUS, start);
            if (match(lexer, '=')) return make_token(lexer, TOKEN_PLUS_EQUAL, start);
            return make_token(lexer, TOKEN_PLUS, start);
        case '-':
            if (match(lexer, '-')) return make_token(lexer, TOKEN_MINUS_MINUS, start);
            if (match(lexer, '=')) return make_token(lexer, TOKEN_MINUS_EQUAL, start);
            return make_token(lexer, TOKEN_MINUS, start);
        case '*':
            if (match(lexer, '=')) return make_token(lexer, TOKEN_STAR_EQUAL, start);
            return make_token(lexer, TOKEN_STAR, start);
        case '/':
            if (match(lexer, '=')) return make_token(lexer, TOKEN_SLASH_EQUAL, start);
            return make_token(lexer, TOKEN_SLASH, start);
        case '%':
            return make_token(lexer, TOKEN_PERCENT, start);

        case '!':
            if (match(lexer, '=')) return make_token(lexer, TOKEN_BANG_EQUAL, start);
            return make_token(lexer, TOKEN_BANG, start);
        case '=':
            if (match(lexer, '=')) return make_token(lexer, TOKEN_EQUAL_EQUAL, start);
            return make_token(lexer, TOKEN_EQUAL, start);
        case '<':
            if (match(lexer, '=')) return make_token(lexer, TOKEN_LESS_EQUAL, start);
            return make_token(lexer, TOKEN_LESS, start);
        case '>':
            if (match(lexer, '=')) return make_token(lexer, TOKEN_GREATER_EQUAL, start);
            return make_token(lexer, TOKEN_GREATER, start);

        case '&':
            if (match(lexer, '&')) return make_token(lexer, TOKEN_AMP_AMP, start);
            break;
        case '|':
            if (match(lexer, '|')) return make_token(lexer, TOKEN_PIPE_PIPE, start);
            break;

        case '"':
        case '\'':
            return scan_string(lexer, c);
    }

    return error_token(lexer, "Bilinmeyen karakter!");
}

const char* token_type_name(TokenType type) {
    switch (type) {
        case TOKEN_LEFT_PAREN: return "(";
        case TOKEN_RIGHT_PAREN: return ")";
        case TOKEN_LEFT_BRACE: return "{";
        case TOKEN_RIGHT_BRACE: return "}";
        case TOKEN_LEFT_BRACKET: return "[";
        case TOKEN_RIGHT_BRACKET: return "]";
        case TOKEN_COMMA: return ",";
        case TOKEN_DOT: return ".";
        case TOKEN_COLON: return ":";
        case TOKEN_SEMICOLON: return ";";
        case TOKEN_PLUS: return "+";
        case TOKEN_MINUS: return "-";
        case TOKEN_STAR: return "*";
        case TOKEN_SLASH: return "/";
        case TOKEN_PERCENT: return "%";
        case TOKEN_PLUS_PLUS: return "++";
        case TOKEN_MINUS_MINUS: return "--";
        case TOKEN_EQUAL: return "=";
        case TOKEN_PLUS_EQUAL: return "+=";
        case TOKEN_MINUS_EQUAL: return "-=";
        case TOKEN_STAR_EQUAL: return "*=";
        case TOKEN_SLASH_EQUAL: return "/=";
        case TOKEN_BANG: return "!";
        case TOKEN_BANG_EQUAL: return "!=";
        case TOKEN_EQUAL_EQUAL: return "==";
        case TOKEN_GREATER: return ">";
        case TOKEN_GREATER_EQUAL: return ">=";
        case TOKEN_LESS: return "<";
        case TOKEN_LESS_EQUAL: return "<=";
        case TOKEN_AMP_AMP: return "&&";
        case TOKEN_PIPE_PIPE: return "||";
        case TOKEN_IDENTIFIER: return "tanımlayıcı";
        case TOKEN_STRING: return "metin";
        case TOKEN_NUMBER: return "sayı";
        case TOKEN_CICEKLI_DEFFUNCTION: return "cicekli.deffunction";
        case TOKEN_DEF: return "def";
        case TOKEN_IF: return "if";
        case TOKEN_ELIF: return "elif";
        case TOKEN_ELSE: return "else";
        case TOKEN_WHILE: return "while";
        case TOKEN_FOR: return "for";
        case TOKEN_IN: return "in";
        case TOKEN_IMPORT: return "import";
        case TOKEN_RETURN: return "return";
        case TOKEN_BREAK: return "break";
        case TOKEN_CONTINUE: return "continue";
        case TOKEN_TRUE: return "true";
        case TOKEN_FALSE: return "false";
        case TOKEN_NULL: return "null";
        case TOKEN_AND: return "and";
        case TOKEN_OR: return "or";
        case TOKEN_NOT: return "not";
        case TOKEN_ERROR: return "hata";
        case TOKEN_EOF: return "EOF";
    }
    return "bilinmeyen";
}
