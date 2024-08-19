#include <ctype.h>
#include <stdio.h>

#include "lexer.h"

static bool sv_eq(SV a, SV b) {
    return a.size == b.size && memcmp(a.data, b.data, b.size) == 0;
}

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

void lexer_buffer(Lexer *lexer, Token token) {
    lexer->peeked = true;
    lexer->buffer = token;
}

static_assert(COUNT_TOKENS == 13, "Update lexer_next()");
bool lexer_next(Lexer *lexer, Token *token) {
    if (lexer->peeked) {
        lexer->peeked = false;
        *token = lexer->buffer;
        return true;
    }

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
            token->type = TOKEN_NOT;
            break;

        default:
            if (!lexer->quiet) {
                fprintf(
                    stderr,
                    PosFmt "error: invalid character '%c'\n",
                    PosArg(token->pos),
                    *token->sv.data);
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
        }
    }

    return true;
}

bool lexer_peek(Lexer *lexer, Token *token) {
    if (!lexer->peeked) {
        bool result = lexer_next(lexer, token);
        lexer_buffer(lexer, *token);
        return result;
    }

    *token = lexer->buffer;
    return true;
}

bool lexer_expect(Lexer *lexer, TokenType type) {
    Token token;
    if (!lexer_next(lexer, &token)) {
        return false;
    }

    if (token.type != type) {
        if (!lexer->quiet) {
            fprintf(
                stderr,
                PosFmt "Error: expected %s, got %s\n",
                PosArg(token.pos),
                token_type_name(type),
                token_type_name(token.type));
        }
        return false;
    }

    return true;
}
