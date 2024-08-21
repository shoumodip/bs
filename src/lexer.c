#include <ctype.h>
#include <stdio.h>

#include "lexer.h"

static void lexer_advance(Lexer *lexer) {
    if (*lexer->sv.data == '\n') {
        if (lexer->sv.size > 1) {
            lexer->pos.row += 1;
            lexer->pos.col = 1;
        }
    } else {
        lexer->pos.col += 1;
    }

    lexer->sv.data += 1;
    lexer->sv.size -= 1;
}

static char lexer_consume(Lexer *lexer) {
    lexer_advance(lexer);
    return lexer->sv.data[-1];
}

static bool lexer_match(Lexer *lexer, char ch) {
    if (lexer->sv.size > 0 && *lexer->sv.data == ch) {
        lexer_advance(lexer);
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

static_assert(COUNT_TOKENS == 32, "Update lexer_next()");
bool lexer_next(Lexer *lexer, Token *token) {
    while (lexer->sv.size > 0) {
        if (isspace(*lexer->sv.data)) {
            lexer_advance(lexer);
        } else if (*lexer->sv.data == '#') {
            while (lexer->sv.size > 0 && *lexer->sv.data != '\n') {
                lexer_advance(lexer);
            }
        } else {
            break;
        }
    }

    token->sv = lexer->sv;
    token->pos = lexer->pos;

    if (lexer->sv.size == 0) {
        token->type = TOKEN_EOF;
    } else if (isdigit(*lexer->sv.data)) {
        token->type = TOKEN_NUM;
        while (lexer->sv.size > 0 && isdigit(*lexer->sv.data)) {
            lexer_advance(lexer);
        }

        if (lexer->sv.size >= 2 && lexer->sv.data[0] == '.' && isdigit(lexer->sv.data[1])) {
            lexer_advance(lexer);
            while (lexer->sv.size > 0 && isdigit(*lexer->sv.data)) {
                lexer_advance(lexer);
            }
        }
    } else if (isalpha(*lexer->sv.data) || *lexer->sv.data == '_') {
        token->type = TOKEN_IDENT;
        while (lexer->sv.size > 0 && (isalnum(*lexer->sv.data) || *lexer->sv.data == '_')) {
            lexer_advance(lexer);
        }
    } else if (lexer_match(lexer, '"')) {
        while (lexer->sv.size > 0 && *lexer->sv.data != '"') {
            lexer_advance(lexer);
        }

        if (lexer->sv.size == 0) {
            if (!lexer->quiet) {
                fprintf(stderr, PosFmt "error: unterminated string\n", PosArg(token->pos));
                lexer->quiet = true;
            }

            token->sv.size -= lexer->sv.size;
            return false;
        }

        token->type = TOKEN_STR;
        lexer_advance(lexer);
    } else {
        switch (lexer_consume(lexer)) {
        case ';':
            token->type = TOKEN_EOL;
            break;

        case '(':
            token->type = TOKEN_LPAREN;
            break;

        case ')':
            token->type = TOKEN_RPAREN;
            break;

        case '{':
            token->type = TOKEN_LBRACE;
            break;

        case '}':
            token->type = TOKEN_RBRACE;
            break;

        case '+':
            token->type = TOKEN_ADD;
            break;

        case '-':
            token->type = TOKEN_SUB;
            break;

        case '*':
            token->type = TOKEN_MUL;
            break;

        case '/':
            token->type = TOKEN_DIV;
            break;

        case '!':
            if (lexer_match(lexer, '=')) {
                token->type = TOKEN_NE;
            } else {
                token->type = TOKEN_NOT;
            }
            break;

        case '>':
            if (lexer_match(lexer, '=')) {
                token->type = TOKEN_GE;
            } else {
                token->type = TOKEN_GT;
            }
            break;

        case '<':
            if (lexer_match(lexer, '=')) {
                token->type = TOKEN_LE;
            } else {
                token->type = TOKEN_LT;
            }
            break;

        case '=':
            if (lexer_match(lexer, '=')) {
                token->type = TOKEN_EQ;
            } else {
                token->type = TOKEN_SET;
            }
            break;

        default:
            if (!lexer->quiet) {
                fprintf(
                    stderr,
                    PosFmt "error: invalid character '%c'\n",
                    PosArg(token->pos),
                    *token->sv.data);
                lexer->quiet = true;
            }

            token->sv.size -= lexer->sv.size;
            return false;
        }
    }

    token->sv.size -= lexer->sv.size;

    if (token->type == TOKEN_IDENT) {
        if (sv_eq(token->sv, SVStatic("nil"))) {
            token->type = TOKEN_NIL;
        } else if (sv_eq(token->sv, SVStatic("true"))) {
            token->type = TOKEN_TRUE;
        } else if (sv_eq(token->sv, SVStatic("false"))) {
            token->type = TOKEN_FALSE;
        } else if (sv_eq(token->sv, SVStatic("or"))) {
            token->type = TOKEN_OR;
        } else if (sv_eq(token->sv, SVStatic("and"))) {
            token->type = TOKEN_AND;
        } else if (sv_eq(token->sv, SVStatic("if"))) {
            token->type = TOKEN_IF;
        } else if (sv_eq(token->sv, SVStatic("else"))) {
            token->type = TOKEN_ELSE;
        } else if (sv_eq(token->sv, SVStatic("for"))) {
            token->type = TOKEN_FOR;
        } else if (sv_eq(token->sv, SVStatic("while"))) {
            token->type = TOKEN_WHILE;
        } else if (sv_eq(token->sv, SVStatic("var"))) {
            token->type = TOKEN_VAR;
        } else if (sv_eq(token->sv, SVStatic("print"))) {
            token->type = TOKEN_PRINT;
        }
    }

    return true;
}
