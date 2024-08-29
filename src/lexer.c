#include <assert.h>
#include <ctype.h>
#include <stdio.h>

#include "lexer.h"

static void lexer_advance(Lexer *l) {
    if (*l->sv.data == '\n') {
        if (l->sv.size > 1) {
            l->loc.row += 1;
            l->loc.col = 1;
        }
    } else {
        l->loc.col += 1;
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
        .loc.path = path,
        .loc.row = 1,
        .loc.col = 1,
    };
}

void lexer_error(Lexer *l) {
    longjmp(l->error, 1);
}

void lexer_buffer(Lexer *l, Token token) {
    l->peeked = true;
    l->buffer = token;
}

static_assert(COUNT_TOKENS == 43, "Update lexer_next()");
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

    Token token = {.sv = l->sv, .loc = l->loc};

    if (l->sv.size == 0) {
        token.type = TOKEN_EOF;
        return token;
    }

    if (isdigit(*l->sv.data)) {
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

        token.sv.size -= l->sv.size;
        return token;
    }

    if (isalpha(*l->sv.data) || *l->sv.data == '_') {
        token.type = TOKEN_IDENT;
        while (l->sv.size > 0 && (isalnum(*l->sv.data) || *l->sv.data == '_')) {
            lexer_advance(l);
        }
        token.sv.size -= l->sv.size;

        if (l->extended) {
            if (sv_eq(token.sv, SVStatic("fr"))) {
                token.type = TOKEN_EOL;
            } else if (sv_eq(token.sv, SVStatic("bruh"))) {
                token.type = TOKEN_NIL;
            } else if (sv_eq(token.sv, SVStatic("nocap"))) {
                token.type = TOKEN_TRUE;
            } else if (sv_eq(token.sv, SVStatic("cap"))) {
                token.type = TOKEN_FALSE;
            } else if (sv_eq(token.sv, SVStatic("or"))) {
                token.type = TOKEN_OR;
            } else if (sv_eq(token.sv, SVStatic("and"))) {
                token.type = TOKEN_AND;
            } else if (sv_eq(token.sv, SVStatic("nah"))) {
                token.type = TOKEN_NOT;
            } else if (sv_eq(token.sv, SVStatic("thicc"))) {
                token.type = TOKEN_LEN;
            } else if (sv_eq(token.sv, SVStatic("redpill"))) {
                token.type = TOKEN_IMPORT;
            } else if (sv_eq(token.sv, SVStatic("be"))) {
                token.type = TOKEN_SET;
            } else if (sv_eq(token.sv, SVStatic("ayo"))) {
                token.type = TOKEN_IF;
            } else if (sv_eq(token.sv, SVStatic("sike"))) {
                token.type = TOKEN_ELSE;
            } else if (sv_eq(token.sv, SVStatic("yall"))) {
                token.type = TOKEN_FOR;
            } else if (sv_eq(token.sv, SVStatic("yolo"))) {
                token.type = TOKEN_WHILE;
            } else if (sv_eq(token.sv, SVStatic("lit"))) {
                token.type = TOKEN_FN;
            } else if (sv_eq(token.sv, SVStatic("fam"))) {
                token.type = TOKEN_PUB;
            } else if (sv_eq(token.sv, SVStatic("mf"))) {
                token.type = TOKEN_VAR;
            } else if (sv_eq(token.sv, SVStatic("bet"))) {
                token.type = TOKEN_RETURN;
            } else if (sv_eq(token.sv, SVStatic("yap"))) {
                token.type = TOKEN_PRINT;
            }
        } else {
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
            } else if (sv_eq(token.sv, SVStatic("len"))) {
                token.type = TOKEN_LEN;
            } else if (sv_eq(token.sv, SVStatic("import"))) {
                token.type = TOKEN_IMPORT;
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
            } else if (sv_eq(token.sv, SVStatic("pub"))) {
                token.type = TOKEN_PUB;
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

    if (lexer_match(l, '"')) {
        while (l->sv.size > 0 && *l->sv.data != '"') {
            lexer_advance(l);
        }

        if (l->sv.size == 0) {
            fprintf(stderr, LocFmt "error: unterminated string\n", LocArg(token.loc));
            lexer_error(l);
        }

        token.type = TOKEN_STR;
        lexer_advance(l);

        token.sv.size -= l->sv.size;
        return token;
    }

    switch (lexer_consume(l)) {
    case ';':
        if (!l->extended) {
            token.type = TOKEN_EOL;
        }
        break;

    case '.':
        if (lexer_match(l, '.')) {
            token.type = TOKEN_JOIN;
        } else {
            token.type = TOKEN_DOT;
        }
        break;

    case ',':
        token.type = TOKEN_COMMA;
        break;

    case '@':
        token.type = TOKEN_NATIVE;
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
        } else if (!l->extended) {
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
        } else if (!l->extended) {
            token.type = TOKEN_SET;
        }
        break;
    }

    if (token.type == TOKEN_EOF) {
        fprintf(
            stderr, LocFmt "error: invalid character '%c'\n", LocArg(token.loc), *token.sv.data);

        lexer_error(l);
    }

    token.sv.size -= l->sv.size;
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
            LocFmt "error: expected %s, got %s\n",
            LocArg(token.loc),
            token_type_name(type, l->extended),
            token_type_name(token.type, l->extended));

        lexer_error(l);
    }
    return token;
}
