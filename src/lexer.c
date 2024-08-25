#include <assert.h>
#include <ctype.h>
#include <stdio.h>

#include "lexer.h"

static void lexer_advance(Lexer *l) {
    if (*l->sv.data == '\n') {
        if (l->sv.size > 1) {
            l->pos.row += 1;
            l->pos.col = 1;
        }
    } else {
        l->pos.col += 1;
    }

    l->sv.data += 1;
    l->sv.size -= 1;
}

static char lexer_consume(Lexer *l) {
    lexer_advance(l);
    return l->sv.data[-1];
}

static bool lexer_match(Lexer *l, char ch) {
    if (l->sv.size > 0 && *l->sv.data == ch) {
        lexer_advance(l);
        return true;
    }
    return false;
}

Lexer lexer_new(const char *path, SV sv) {
    return (Lexer){
        .sv = sv,
        .pos.path = path,
        .pos.row = 1,
        .pos.col = 1,
    };
}

void lexer_error(Lexer *l) {
    longjmp(l->error, 1);
}

void lexer_buffer(Lexer *l, Token token) {
    l->peeked = true;
    l->buffer = token;
}

static_assert(COUNT_TOKENS == 38, "Update lexer_next()");
Token lexer_next(Lexer *l) {
    if (l->peeked) {
        l->peeked = false;
        return l->buffer;
    }

    while (l->sv.size > 0) {
        if (isspace(*l->sv.data)) {
            lexer_advance(l);
        } else if (*l->sv.data == '#') {
            while (l->sv.size > 0 && *l->sv.data != '\n') {
                lexer_advance(l);
            }
        } else {
            break;
        }
    }

    Token token = {.sv = l->sv, .pos = l->pos};

    if (l->sv.size == 0) {
        token.type = TOKEN_EOF;
    } else if (isdigit(*l->sv.data)) {
        token.type = TOKEN_NUM;
        while (l->sv.size > 0 && isdigit(*l->sv.data)) {
            lexer_advance(l);
        }

        if (l->sv.size >= 2 && l->sv.data[0] == '.' && isdigit(l->sv.data[1])) {
            lexer_advance(l);
            while (l->sv.size > 0 && isdigit(*l->sv.data)) {
                lexer_advance(l);
            }
        }
    } else if (isalpha(*l->sv.data) || *l->sv.data == '_') {
        token.type = TOKEN_IDENT;
        while (l->sv.size > 0 && (isalnum(*l->sv.data) || *l->sv.data == '_')) {
            lexer_advance(l);
        }
    } else if (lexer_match(l, '"')) {
        while (l->sv.size > 0 && *l->sv.data != '"') {
            lexer_advance(l);
        }

        if (l->sv.size == 0) {
            fprintf(stderr, PosFmt "error: unterminated string\n", PosArg(token.pos));
            lexer_error(l);
        }

        token.type = TOKEN_STR;
        lexer_advance(l);
    } else {
        switch (lexer_consume(l)) {
        case ';':
            token.type = TOKEN_EOL;
            break;

        case '.':
            token.type = TOKEN_DOT;
            break;

        case ',':
            token.type = TOKEN_COMMA;
            break;

        case '(':
            token.type = TOKEN_LPAREN;
            break;

        case ')':
            token.type = TOKEN_RPAREN;
            break;

        case '{':
            token.type = TOKEN_LBRACE;
            break;

        case '}':
            token.type = TOKEN_RBRACE;
            break;

        case '[':
            token.type = TOKEN_LBRACKET;
            break;

        case ']':
            token.type = TOKEN_RBRACKET;
            break;

        case '+':
            token.type = TOKEN_ADD;
            break;

        case '-':
            token.type = TOKEN_SUB;
            break;

        case '*':
            token.type = TOKEN_MUL;
            break;

        case '/':
            token.type = TOKEN_DIV;
            break;

        case '!':
            if (lexer_match(l, '=')) {
                token.type = TOKEN_NE;
            } else {
                token.type = TOKEN_NOT;
            }
            break;

        case '>':
            if (lexer_match(l, '=')) {
                token.type = TOKEN_GE;
            } else {
                token.type = TOKEN_GT;
            }
            break;

        case '<':
            if (lexer_match(l, '=')) {
                token.type = TOKEN_LE;
            } else {
                token.type = TOKEN_LT;
            }
            break;

        case '=':
            if (lexer_match(l, '=')) {
                token.type = TOKEN_EQ;
            } else {
                token.type = TOKEN_SET;
            }
            break;

        default:
            fprintf(
                stderr,
                PosFmt "error: invalid character '%c'\n",
                PosArg(token.pos),
                *token.sv.data);

            lexer_error(l);
        }
    }

    token.sv.size -= l->sv.size;
    if (token.type == TOKEN_IDENT) {
        if (sv_eq(token.sv, SVStatic("nil"))) {
            token.type = TOKEN_NIL;
        } else if (sv_eq(token.sv, SVStatic("true"))) {
            token.type = TOKEN_TRUE;
        } else if (sv_eq(token.sv, SVStatic("false"))) {
            token.type = TOKEN_FALSE;
        } else if (sv_eq(token.sv, SVStatic("or"))) {
            token.type = TOKEN_OR;
        } else if (sv_eq(token.sv, SVStatic("and"))) {
            token.type = TOKEN_AND;
        } else if (sv_eq(token.sv, SVStatic("if"))) {
            token.type = TOKEN_IF;
        } else if (sv_eq(token.sv, SVStatic("else"))) {
            token.type = TOKEN_ELSE;
        } else if (sv_eq(token.sv, SVStatic("for"))) {
            token.type = TOKEN_FOR;
        } else if (sv_eq(token.sv, SVStatic("while"))) {
            token.type = TOKEN_WHILE;
        } else if (sv_eq(token.sv, SVStatic("fn"))) {
            token.type = TOKEN_FN;
        } else if (sv_eq(token.sv, SVStatic("var"))) {
            token.type = TOKEN_VAR;
        } else if (sv_eq(token.sv, SVStatic("return"))) {
            token.type = TOKEN_RETURN;
        } else if (sv_eq(token.sv, SVStatic("print"))) {
            token.type = TOKEN_PRINT;
        }
    }

    return token;
}

Token lexer_peek(Lexer *l) {
    if (!l->peeked) {
        lexer_buffer(l, lexer_next(l));
    }
    return l->buffer;
}

bool lexer_read(Lexer *l, TokenType type) {
    lexer_peek(l);
    l->peeked = l->buffer.type != type;
    return !l->peeked;
}

Token lexer_expect(Lexer *l, TokenType type) {
    const Token token = lexer_next(l);
    if (token.type != type) {
        fprintf(
            stderr,
            PosFmt "error: expected %s, got %s\n",
            PosArg(token.pos),
            token_type_name(type),
            token_type_name(token.type));

        lexer_error(l);
    }
    return token;
}
