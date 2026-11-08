#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void error_at(Parser* parser, Token* token, const char* message) {
    if (parser->panic_mode) return;
    parser->panic_mode = true;
    parser->had_error = true;

    fprintf(stderr, "MeteScript Sözdizimi Hatası (Satır %d): %s", token->line, message);
    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " (Dosya sonu)\n");
    } else if (token->type == TOKEN_ERROR) {
        fprintf(stderr, "\n");
    } else {
        fprintf(stderr, " ('%.*s' yakınında)\n", (int)token->length, token->start);
    }
}

static void error_current(Parser* parser, const char* message) {
    error_at(parser, &parser->current, message);
}

static void advance(Parser* parser) {
    if (parser->previous.string_value) {
        free(parser->previous.string_value);
        parser->previous.string_value = NULL;
    }
    parser->previous = parser->current;

    for (;;) {
        parser->current = lexer_next_token(&parser->lexer);
        if (parser->current.type != TOKEN_ERROR) break;
        error_current(parser, parser->current.start);
    }
}

static bool check(Parser* parser, TokenType type) {
    return parser->current.type == type;
}

static bool match(Parser* parser, TokenType type) {
    if (!check(parser, type)) return false;
    advance(parser);
    return true;
}

static void consume(Parser* parser, TokenType type, const char* message) {
    if (parser->current.type == type) {
        advance(parser);
        return;
    }
    error_current(parser, message);
}

void parser_init(Parser* parser, const char* source) {
    lexer_init(&parser->lexer, source);
    parser->previous.string_value = NULL;
    parser->current.string_value = NULL;
    parser->had_error = false;
    parser->panic_mode = false;
    advance(parser);
}

static Expr* parse_expression(Parser* parser);
static Stmt* parse_block(Parser* parser);

static Expr* parse_primary(Parser* parser) {
    int line = parser->current.line;

    if (match(parser, TOKEN_NUMBER)) {
        return make_literal_expr(make_number(parser->previous.number_value), line);
    }

    if (match(parser, TOKEN_STRING)) {
        ObjString* str = copy_string(parser->previous.string_value ? parser->previous.string_value : "");
        return make_literal_expr(make_obj((Obj*)str), line);
    }

    if (match(parser, TOKEN_TRUE)) {
        return make_literal_expr(make_bool(true), line);
    }

    if (match(parser, TOKEN_FALSE)) {
        return make_literal_expr(make_bool(false), line);
    }

    if (match(parser, TOKEN_NULL)) {
        return make_literal_expr(make_null(), line);
    }

    if (match(parser, TOKEN_IDENTIFIER)) {
        char name_buf[256];
        size_t len = parser->previous.length < 255 ? parser->previous.length : 255;
        memcpy(name_buf, parser->previous.start, len);
        name_buf[len] = '\0';
        return make_identifier_expr(name_buf, line);
    }

    if (match(parser, TOKEN_LEFT_PAREN)) {
        Expr* expr = parse_expression(parser);
        consume(parser, TOKEN_RIGHT_PAREN, "')' bekleniyor!");
        return expr;
    }

    // List literal: [elem1, elem2, ...]
    if (match(parser, TOKEN_LEFT_BRACKET)) {
        size_t cap = 8;
        size_t count = 0;
        Expr** elements = (Expr**)malloc(sizeof(Expr*) * cap);

        if (!check(parser, TOKEN_RIGHT_BRACKET)) {
            do {
                if (count >= cap) {
                    cap *= 2;
                    elements = (Expr**)realloc(elements, sizeof(Expr*) * cap);
                }
                elements[count++] = parse_expression(parser);
            } while (match(parser, TOKEN_COMMA));
        }
        consume(parser, TOKEN_RIGHT_BRACKET, "']' bekleniyor!");
        return make_list_expr(elements, count, line);
    }

    // Map literal: { "key": val, ... }
    if (match(parser, TOKEN_LEFT_BRACE)) {
        size_t cap = 8;
        size_t count = 0;
        AstMapEntry* entries = (AstMapEntry*)malloc(sizeof(AstMapEntry) * cap);

        if (!check(parser, TOKEN_RIGHT_BRACE)) {
            do {
                char* key = NULL;
                if (match(parser, TOKEN_STRING)) {
                    key = strdup(parser->previous.string_value ? parser->previous.string_value : "");
                } else if (match(parser, TOKEN_IDENTIFIER)) {
                    char kbuf[256];
                    size_t klen = parser->previous.length < 255 ? parser->previous.length : 255;
                    memcpy(kbuf, parser->previous.start, klen);
                    kbuf[klen] = '\0';
                    key = strdup(kbuf);
                } else {
                    error_current(parser, "Harita (map/sözlük) anahtarı metin veya tanımlayıcı olmalıdır!");
                    break;
                }

                consume(parser, TOKEN_COLON, "Anahtardan sonra ':' bekleniyor!");
                Expr* val = parse_expression(parser);

                if (count >= cap) {
                    cap *= 2;
                    entries = (AstMapEntry*)realloc(entries, sizeof(AstMapEntry) * cap);
                }
                entries[count].key = key;
                entries[count].value = val;
                count++;
            } while (match(parser, TOKEN_COMMA));
        }
        consume(parser, TOKEN_RIGHT_BRACE, "'}' bekleniyor!");
        return make_map_expr(entries, count, line);
    }

    // Function expression: mete.deffunction(args) { body } or def(args) { body }
    if (match(parser, TOKEN_METE_DEFFUNCTION) || match(parser, TOKEN_DEF)) {
        consume(parser, TOKEN_LEFT_PAREN, "Fonksiyon parametreleri için '(' bekleniyor!");
        size_t cap = 4;
        size_t arity = 0;
        char** params = (char**)malloc(sizeof(char*) * cap);

        if (!check(parser, TOKEN_RIGHT_PAREN)) {
            do {
                consume(parser, TOKEN_IDENTIFIER, "Parametre ismi bekleniyor!");
                if (arity >= cap) {
                    cap *= 2;
                    params = (char**)realloc(params, sizeof(char*) * cap);
                }
                char pbuf[256];
                size_t plen = parser->previous.length < 255 ? parser->previous.length : 255;
                memcpy(pbuf, parser->previous.start, plen);
                pbuf[plen] = '\0';
                params[arity++] = strdup(pbuf);
            } while (match(parser, TOKEN_COMMA));
        }
        consume(parser, TOKEN_RIGHT_PAREN, "')' bekleniyor!");
        Stmt* body = parse_block(parser);
        return make_function_expr(params, arity, body, line);
    }

    error_current(parser, "Geçersiz ifade!");
    return make_literal_expr(make_null(), line);
}

static Expr* parse_call_member_index(Parser* parser) {
    Expr* expr = parse_primary(parser);

    for (;;) {
        if (match(parser, TOKEN_LEFT_PAREN)) {
            // Function call
            size_t cap = 4;
            size_t count = 0;
            Expr** args = (Expr**)malloc(sizeof(Expr*) * cap);

            if (!check(parser, TOKEN_RIGHT_PAREN)) {
                do {
                    if (count >= cap) {
                        cap *= 2;
                        args = (Expr**)realloc(args, sizeof(Expr*) * cap);
                    }
                    args[count++] = parse_expression(parser);
                } while (match(parser, TOKEN_COMMA));
            }
            consume(parser, TOKEN_RIGHT_PAREN, "')' bekleniyor!");
            expr = make_call_expr(expr, args, count, parser->previous.line);
        } else if (match(parser, TOKEN_DOT)) {
            // Member access: obj.field
            consume(parser, TOKEN_IDENTIFIER, "Özellik adı bekleniyor!");
            char member_buf[256];
            size_t len = parser->previous.length < 255 ? parser->previous.length : 255;
            memcpy(member_buf, parser->previous.start, len);
            member_buf[len] = '\0';
            expr = make_member_expr(expr, member_buf, parser->previous.line);
        } else if (match(parser, TOKEN_LEFT_BRACKET)) {
            // Index access: obj[idx]
            Expr* index = parse_expression(parser);
            consume(parser, TOKEN_RIGHT_BRACKET, "']' bekleniyor!");
            expr = make_index_expr(expr, index, parser->previous.line);
        } else {
            break;
        }
    }

    return expr;
}

static Expr* parse_unary(Parser* parser) {
    if (match(parser, TOKEN_BANG) || match(parser, TOKEN_NOT) || match(parser, TOKEN_MINUS)) {
        TokenType op = parser->previous.type;
        int line = parser->previous.line;
        Expr* right = parse_unary(parser);
        return make_unary_expr(op, right, line);
    }
    return parse_call_member_index(parser);
}

static Expr* parse_factor(Parser* parser) {
    Expr* expr = parse_unary(parser);
    while (match(parser, TOKEN_STAR) || match(parser, TOKEN_SLASH) || match(parser, TOKEN_PERCENT)) {
        TokenType op = parser->previous.type;
        int line = parser->previous.line;
        Expr* right = parse_unary(parser);
        expr = make_binary_expr(expr, op, right, line);
    }
    return expr;
}

static Expr* parse_term(Parser* parser) {
    Expr* expr = parse_factor(parser);
    while (match(parser, TOKEN_PLUS) || match(parser, TOKEN_MINUS)) {
        TokenType op = parser->previous.type;
        int line = parser->previous.line;
        Expr* right = parse_factor(parser);
        expr = make_binary_expr(expr, op, right, line);
    }
    return expr;
}

static Expr* parse_comparison(Parser* parser) {
    Expr* expr = parse_term(parser);
    while (match(parser, TOKEN_GREATER) || match(parser, TOKEN_GREATER_EQUAL) ||
           match(parser, TOKEN_LESS) || match(parser, TOKEN_LESS_EQUAL)) {
        TokenType op = parser->previous.type;
        int line = parser->previous.line;
        Expr* right = parse_term(parser);
        expr = make_binary_expr(expr, op, right, line);
    }
    return expr;
}

static Expr* parse_equality(Parser* parser) {
    Expr* expr = parse_comparison(parser);
    while (match(parser, TOKEN_EQUAL_EQUAL) || match(parser, TOKEN_BANG_EQUAL)) {
        TokenType op = parser->previous.type;
        int line = parser->previous.line;
        Expr* right = parse_comparison(parser);
        expr = make_binary_expr(expr, op, right, line);
    }
    return expr;
}

static Expr* parse_logical_and(Parser* parser) {
    Expr* expr = parse_equality(parser);
    while (match(parser, TOKEN_AND) || match(parser, TOKEN_AMP_AMP)) {
        TokenType op = parser->previous.type;
        int line = parser->previous.line;
        Expr* right = parse_equality(parser);
        expr = make_binary_expr(expr, op, right, line);
    }
    return expr;
}

static Expr* parse_logical_or(Parser* parser) {
    Expr* expr = parse_logical_and(parser);
    while (match(parser, TOKEN_OR) || match(parser, TOKEN_PIPE_PIPE)) {
        TokenType op = parser->previous.type;
        int line = parser->previous.line;
        Expr* right = parse_logical_and(parser);
        expr = make_binary_expr(expr, op, right, line);
    }
    return expr;
}

static Expr* parse_assignment(Parser* parser) {
    Expr* expr = parse_logical_or(parser);

    if (match(parser, TOKEN_EQUAL) || match(parser, TOKEN_PLUS_EQUAL) ||
        match(parser, TOKEN_MINUS_EQUAL) || match(parser, TOKEN_STAR_EQUAL) ||
        match(parser, TOKEN_SLASH_EQUAL)) {
        TokenType op = parser->previous.type;
        int line = parser->previous.line;
        Expr* value = parse_assignment(parser);

        if (expr->type == EXPR_IDENTIFIER) {
            char* name = expr->as.identifier;
            return make_assign_expr(name, op, value, line);
        } else if (expr->type == EXPR_MEMBER) {
            Expr* obj = expr->as.member.object;
            char* member = expr->as.member.member;
            return make_member_assign_expr(obj, member, value, line);
        } else if (expr->type == EXPR_INDEX) {
            Expr* obj = expr->as.index.object;
            Expr* idx = expr->as.index.index;
            return make_index_assign_expr(obj, idx, value, line);
        }

        error_at(parser, &parser->previous, "Geçersiz atama hedefi!");
    }

    return expr;
}

static Expr* parse_expression(Parser* parser) {
    return parse_assignment(parser);
}

static Stmt* parse_block(Parser* parser) {
    int line = parser->current.line;
    consume(parser, TOKEN_LEFT_BRACE, "'{' bekleniyor!");

    size_t cap = 8;
    size_t count = 0;
    Stmt** stmts = (Stmt**)malloc(sizeof(Stmt*) * cap);

    while (!check(parser, TOKEN_RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
        if (count >= cap) {
            cap *= 2;
            stmts = (Stmt**)realloc(stmts, sizeof(Stmt*) * cap);
        }
        stmts[count++] = parse_statement(parser);
    }

    consume(parser, TOKEN_RIGHT_BRACE, "'}' bekleniyor!");
    return make_block_stmt(stmts, count, line);
}

static Stmt* parse_function_statement(Parser* parser) {
    int line = parser->previous.line;

    consume(parser, TOKEN_IDENTIFIER, "Fonksiyon ismi bekleniyor!");
    char name_buf[256];
    size_t len = parser->previous.length < 255 ? parser->previous.length : 255;
    memcpy(name_buf, parser->previous.start, len);
    name_buf[len] = '\0';

    consume(parser, TOKEN_LEFT_PAREN, "Fonksiyon parametreleri için '(' bekleniyor!");
    size_t cap = 4;
    size_t arity = 0;
    char** params = (char**)malloc(sizeof(char*) * cap);

    if (!check(parser, TOKEN_RIGHT_PAREN)) {
        do {
            consume(parser, TOKEN_IDENTIFIER, "Parametre ismi bekleniyor!");
            if (arity >= cap) {
                cap *= 2;
                params = (char**)realloc(params, sizeof(char*) * cap);
            }
            char pbuf[256];
            size_t plen = parser->previous.length < 255 ? parser->previous.length : 255;
            memcpy(pbuf, parser->previous.start, plen);
            pbuf[plen] = '\0';
            params[arity++] = strdup(pbuf);
        } while (match(parser, TOKEN_COMMA));
    }
    consume(parser, TOKEN_RIGHT_PAREN, "')' bekleniyor!");

    Stmt* body = parse_block(parser);
    match(parser, TOKEN_SEMICOLON); // Optional ';' after function block
    return make_function_stmt(name_buf, params, arity, body, line);
}

static Stmt* parse_if_statement(Parser* parser) {
    int line = parser->previous.line;

    bool has_paren = match(parser, TOKEN_LEFT_PAREN);
    Expr* cond = parse_expression(parser);
    if (has_paren) {
        consume(parser, TOKEN_RIGHT_PAREN, "')' bekleniyor!");
    }

    Stmt* then_branch = parse_block(parser);

    size_t cap = 2;
    size_t elif_count = 0;
    ElifClause* elifs = (ElifClause*)malloc(sizeof(ElifClause) * cap);

    while (match(parser, TOKEN_ELIF)) {
        bool elif_paren = match(parser, TOKEN_LEFT_PAREN);
        Expr* elif_cond = parse_expression(parser);
        if (elif_paren) {
            consume(parser, TOKEN_RIGHT_PAREN, "')' bekleniyor!");
        }
        Stmt* elif_body = parse_block(parser);

        if (elif_count >= cap) {
            cap *= 2;
            elifs = (ElifClause*)realloc(elifs, sizeof(ElifClause) * cap);
        }
        elifs[elif_count].condition = elif_cond;
        elifs[elif_count].body = elif_body;
        elif_count++;
    }

    Stmt* else_branch = NULL;
    if (match(parser, TOKEN_ELSE)) {
        else_branch = parse_block(parser);
    }

    match(parser, TOKEN_SEMICOLON); // Optional ';' after if block
    return make_if_stmt(cond, then_branch, elifs, elif_count, else_branch, line);
}

static Stmt* parse_while_statement(Parser* parser) {
    int line = parser->previous.line;

    bool has_paren = match(parser, TOKEN_LEFT_PAREN);
    Expr* cond = parse_expression(parser);
    if (has_paren) {
        consume(parser, TOKEN_RIGHT_PAREN, "')' bekleniyor!");
    }

    Stmt* body = parse_block(parser);
    match(parser, TOKEN_SEMICOLON); // Optional ';' after while block
    return make_while_stmt(cond, body, line);
}

static Stmt* parse_for_statement(Parser* parser) {
    int line = parser->previous.line;
    bool has_paren = match(parser, TOKEN_LEFT_PAREN);

    // Check if Python-style: for (var in iter) or for var in iter
    if (check(parser, TOKEN_IDENTIFIER)) {
        Token var_token = parser->current;
        // Lookahead to see if next is 'in'
        Lexer lookahead = parser->lexer;
        Token next_tok = lexer_next_token(&lookahead);

        if (next_tok.type == TOKEN_IN) {
            advance(parser); // consume identifier
            char var_buf[256];
            size_t len = var_token.length < 255 ? var_token.length : 255;
            memcpy(var_buf, var_token.start, len);
            var_buf[len] = '\0';

            advance(parser); // consume 'in'
            Expr* iter = parse_expression(parser);
            if (has_paren) {
                consume(parser, TOKEN_RIGHT_PAREN, "')' bekleniyor!");
            }
            Stmt* body = parse_block(parser);
            match(parser, TOKEN_SEMICOLON);
            return make_for_in_stmt(var_buf, iter, body, line);
        }
    }

    // Otherwise C-style: for (init; cond; step)
    Stmt* init = NULL;
    if (match(parser, TOKEN_SEMICOLON)) {
        init = NULL;
    } else {
        Expr* init_expr = parse_expression(parser);
        consume(parser, TOKEN_SEMICOLON, "For döngüsü başlangıcından sonra ';' bekleniyor!");
        init = make_expr_stmt(init_expr, line);
    }

    Expr* cond = NULL;
    if (!check(parser, TOKEN_SEMICOLON)) {
        cond = parse_expression(parser);
    }
    consume(parser, TOKEN_SEMICOLON, "For döngüsü koşulundan sonra ';' bekleniyor!");

    Expr* step = NULL;
    if (!check(parser, TOKEN_RIGHT_PAREN)) {
        step = parse_expression(parser);
    }
    if (has_paren) {
        consume(parser, TOKEN_RIGHT_PAREN, "')' bekleniyor!");
    }

    Stmt* body = parse_block(parser);
    match(parser, TOKEN_SEMICOLON);
    return make_for_c_stmt(init, cond, step, body, line);
}

Stmt* parse_statement(Parser* parser) {
    int line = parser->current.line;

    if (check(parser, TOKEN_METE_DEFFUNCTION) || check(parser, TOKEN_DEF)) {
        Lexer lookahead = parser->lexer;
        Token next_tok = lexer_next_token(&lookahead);
        if (next_tok.type == TOKEN_IDENTIFIER) {
            advance(parser); // consume mete.deffunction or def
            return parse_function_statement(parser);
        }
    }

    if (match(parser, TOKEN_IF)) {
        return parse_if_statement(parser);
    }

    if (match(parser, TOKEN_WHILE)) {
        return parse_while_statement(parser);
    }

    if (match(parser, TOKEN_FOR)) {
        return parse_for_statement(parser);
    }

    if (match(parser, TOKEN_RETURN)) {
        Expr* expr = NULL;
        if (!check(parser, TOKEN_SEMICOLON)) {
            expr = parse_expression(parser);
        }
        consume(parser, TOKEN_SEMICOLON, "Return ifadesinin sonuna ';' eklenmelidir!");
        return make_return_stmt(expr, line);
    }

    if (match(parser, TOKEN_BREAK)) {
        consume(parser, TOKEN_SEMICOLON, "Break ifadesinin sonuna ';' eklenmelidir!");
        return make_break_stmt(line);
    }

    if (match(parser, TOKEN_CONTINUE)) {
        consume(parser, TOKEN_SEMICOLON, "Continue ifadesinin sonuna ';' eklenmelidir!");
        return make_continue_stmt(line);
    }

    if (check(parser, TOKEN_LEFT_BRACE)) {
        Stmt* block = parse_block(parser);
        match(parser, TOKEN_SEMICOLON);
        return block;
    }

    // Expression statement
    Expr* expr = parse_expression(parser);
    consume(parser, TOKEN_SEMICOLON, "İfade sonuna ';' eklenmelidir!");
    return make_expr_stmt(expr, line);
}

Stmt** parse_program(Parser* parser, size_t* out_count) {
    size_t cap = 16;
    size_t count = 0;
    Stmt** stmts = (Stmt**)malloc(sizeof(Stmt*) * cap);

    while (!check(parser, TOKEN_EOF)) {
        if (count >= cap) {
            cap *= 2;
            stmts = (Stmt**)realloc(stmts, sizeof(Stmt*) * cap);
        }
        stmts[count++] = parse_statement(parser);
        if (parser->panic_mode) {
            // Synchronize on error
            parser->panic_mode = false;
            while (!check(parser, TOKEN_EOF)) {
                if (parser->previous.type == TOKEN_SEMICOLON) break;
                switch (parser->current.type) {
                    case TOKEN_METE_DEFFUNCTION:
                    case TOKEN_DEF:
                    case TOKEN_IF:
                    case TOKEN_WHILE:
                    case TOKEN_FOR:
                    case TOKEN_RETURN:
                        goto synchronized;
                    default:
                        break;
                }
                advance(parser);
            }
            synchronized:;
        }
    }

    if (parser->current.string_value) {
        free(parser->current.string_value);
        parser->current.string_value = NULL;
    }
    if (parser->previous.string_value) {
        free(parser->previous.string_value);
        parser->previous.string_value = NULL;
    }

    *out_count = count;
    return stmts;
}
