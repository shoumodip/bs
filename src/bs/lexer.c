#include <assert.h>
#include <ctype.h>

#include "bs/lexer.h"

static void bs_lexer_advance(Bs_Lexer *l) {
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

static char bs_lexer_consume(Bs_Lexer *l) {
    bs_lexer_advance(l);
    return l->sv.data[-1];
}

static bool bs_lexer_match(Bs_Lexer *l, char ch) {
    if (l->sv.size > 0 && *l->sv.data == ch) {
        bs_lexer_advance(l);
        return true;
    }
    return false;
}

Bs_Lexer bs_lexer_new(const char *path, Bs_Sv input, Bs_Writer *error) {
    return (Bs_Lexer){
        .sv = input,
        .loc.path = path,
        .loc.row = 1,
        .loc.col = 1,
        .error = error,
    };
}

void bs_lexer_error(Bs_Lexer *l) {
    longjmp(l->unwind, 1);
}

void bs_lexer_buffer(Bs_Lexer *l, Bs_Token token) {
    l->peeked = true;
    l->buffer = token;
}

Bs_Token bs_lexer_str(Bs_Lexer *l, Bs_Loc loc) {
    Bs_Token token = {.sv = l->sv, .loc = loc};

    while (l->sv.size > 0 && *l->sv.data != '"') {
        char ch = *l->sv.data;
        if (ch == '\\') {
            if (l->sv.size <= 2) {
                bs_lexer_advance(l);
                bs_lexer_advance(l);
                break;
            }

            switch (l->sv.data[1]) {
            case 'n':
            case 't':
            case '0':
            case '"':
            case '\\':
                break;

            case '(':
                token.type = BS_TOKEN_ISTR;
                token.sv.size -= l->sv.size;

                bs_lexer_advance(l);
                return token;

            default:
                bs_fmt(
                    l->error, Bs_Loc_Fmt "error: invalid escape character\n", Bs_Loc_Arg(l->loc));

                bs_lexer_error(l);
            }

            bs_lexer_advance(l);
        }

        bs_lexer_advance(l);
    }

    if (!l->sv.size) {
        bs_fmt(l->error, Bs_Loc_Fmt "error: unterminated string\n", Bs_Loc_Arg(l->loc));
        bs_lexer_error(l);
    }

    token.type = BS_TOKEN_STR;
    token.sv.size -= l->sv.size;

    bs_lexer_advance(l);
    return token;
}

static_assert(BS_COUNT_TOKENS == 46, "Update bs_lexer_next()");
Bs_Token bs_lexer_next(Bs_Lexer *l) {
    if (l->peeked) {
        l->peeked = false;
        return l->buffer;
    }

    while (l->sv.size > 0) {
        if (isspace(*l->sv.data)) {
            bs_lexer_advance(l);
        } else if (*l->sv.data == '#') {
            while (l->sv.size > 0 && *l->sv.data != '\n') {
                bs_lexer_advance(l);
            }
        } else {
            break;
        }
    }

    Bs_Token token = {.sv = l->sv, .loc = l->loc};
    if (l->sv.size == 0) {
        return token;
    }

    if (isdigit(*l->sv.data)) {
        token.type = BS_TOKEN_NUM;
        while (l->sv.size > 0 && isdigit(*l->sv.data)) {
            bs_lexer_advance(l);
        }

        if (l->sv.size >= 2 && l->sv.data[0] == '.' && isdigit(l->sv.data[1])) {
            bs_lexer_advance(l);
            while (l->sv.size > 0 && isdigit(*l->sv.data)) {
                bs_lexer_advance(l);
            }
        }

        token.sv.size -= l->sv.size;
        return token;
    }

    if (isalpha(*l->sv.data) || *l->sv.data == '_') {
        token.type = BS_TOKEN_IDENT;
        while (l->sv.size > 0 && (isalnum(*l->sv.data) || *l->sv.data == '_')) {
            bs_lexer_advance(l);
        }
        token.sv.size -= l->sv.size;

        if (l->extended) {
            if (bs_sv_eq(token.sv, Bs_Sv_Static("fr"))) {
                token.type = BS_TOKEN_EOL;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("bruh"))) {
                token.type = BS_TOKEN_NIL;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("nocap"))) {
                token.type = BS_TOKEN_TRUE;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("cap"))) {
                token.type = BS_TOKEN_FALSE;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("or"))) {
                token.type = BS_TOKEN_OR;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("and"))) {
                token.type = BS_TOKEN_AND;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("nah"))) {
                token.type = BS_TOKEN_NOT;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("thicc"))) {
                token.type = BS_TOKEN_LEN;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("redpill"))) {
                token.type = BS_TOKEN_IMPORT;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("be"))) {
                token.type = BS_TOKEN_SET;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("ayo"))) {
                token.type = BS_TOKEN_IF;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("sike"))) {
                token.type = BS_TOKEN_ELSE;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("amongus"))) {
                token.type = BS_TOKEN_IN;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("yall"))) {
                token.type = BS_TOKEN_FOR;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("yolo"))) {
                token.type = BS_TOKEN_WHILE;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("yeet"))) {
                token.type = BS_TOKEN_BREAK;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("slickback"))) {
                token.type = BS_TOKEN_CONTINUE;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("lit"))) {
                token.type = BS_TOKEN_FN;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("fam"))) {
                token.type = BS_TOKEN_PUB;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("mf"))) {
                token.type = BS_TOKEN_VAR;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("bet"))) {
                token.type = BS_TOKEN_RETURN;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("is_big_boss"))) {
                token.type = BS_TOKEN_IS_MAIN_MODULE;
            }
        } else {
            if (bs_sv_eq(token.sv, Bs_Sv_Static("nil"))) {
                token.type = BS_TOKEN_NIL;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("true"))) {
                token.type = BS_TOKEN_TRUE;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("false"))) {
                token.type = BS_TOKEN_FALSE;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("or"))) {
                token.type = BS_TOKEN_OR;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("and"))) {
                token.type = BS_TOKEN_AND;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("len"))) {
                token.type = BS_TOKEN_LEN;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("import"))) {
                token.type = BS_TOKEN_IMPORT;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("if"))) {
                token.type = BS_TOKEN_IF;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("else"))) {
                token.type = BS_TOKEN_ELSE;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("in"))) {
                token.type = BS_TOKEN_IN;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("for"))) {
                token.type = BS_TOKEN_FOR;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("while"))) {
                token.type = BS_TOKEN_WHILE;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("break"))) {
                token.type = BS_TOKEN_BREAK;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("continue"))) {
                token.type = BS_TOKEN_CONTINUE;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("fn"))) {
                token.type = BS_TOKEN_FN;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("pub"))) {
                token.type = BS_TOKEN_PUB;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("var"))) {
                token.type = BS_TOKEN_VAR;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("return"))) {
                token.type = BS_TOKEN_RETURN;
            } else if (bs_sv_eq(token.sv, Bs_Sv_Static("is_main_module"))) {
                token.type = BS_TOKEN_IS_MAIN_MODULE;
            }
        }

        return token;
    }

    if (bs_lexer_match(l, '"')) {
        return bs_lexer_str(l, token.loc);
    }

    switch (bs_lexer_consume(l)) {
    case ';':
        if (!l->extended) {
            token.type = BS_TOKEN_EOL;
        }
        break;

    case '.':
        if (bs_lexer_match(l, '.')) {
            token.type = BS_TOKEN_JOIN;
        } else {
            token.type = BS_TOKEN_DOT;
        }
        break;

    case ',':
        token.type = BS_TOKEN_COMMA;
        break;

    case '(':
        token.type = BS_TOKEN_LPAREN;
        break;

    case ')':
        token.type = BS_TOKEN_RPAREN;
        break;

    case '{':
        token.type = BS_TOKEN_LBRACE;
        break;

    case '}':
        token.type = BS_TOKEN_RBRACE;
        break;

    case '[':
        token.type = BS_TOKEN_LBRACKET;
        break;

    case ']':
        token.type = BS_TOKEN_RBRACKET;
        break;

    case '+':
        token.type = BS_TOKEN_ADD;
        break;

    case '-':
        token.type = BS_TOKEN_SUB;
        break;

    case '*':
        token.type = BS_TOKEN_MUL;
        break;

    case '/':
        token.type = BS_TOKEN_DIV;
        break;

    case '!':
        if (bs_lexer_match(l, '=')) {
            token.type = BS_TOKEN_NE;
        } else if (!l->extended) {
            token.type = BS_TOKEN_NOT;
        }
        break;

    case '>':
        if (bs_lexer_match(l, '=')) {
            token.type = BS_TOKEN_GE;
        } else {
            token.type = BS_TOKEN_GT;
        }
        break;

    case '<':
        if (bs_lexer_match(l, '=')) {
            token.type = BS_TOKEN_LE;
        } else {
            token.type = BS_TOKEN_LT;
        }
        break;

    case '=':
        if (bs_lexer_match(l, '=')) {
            token.type = BS_TOKEN_EQ;
        } else if (!l->extended) {
            token.type = BS_TOKEN_SET;
        }
        break;
    }

    if (token.type == BS_TOKEN_EOF) {
        bs_fmt(
            l->error,
            Bs_Loc_Fmt "error: invalid character '%c'\n",
            Bs_Loc_Arg(token.loc),
            *token.sv.data);

        bs_lexer_error(l);
    }

    token.sv.size -= l->sv.size;
    return token;
}

Bs_Token bs_lexer_peek(Bs_Lexer *l) {
    if (!l->peeked) {
        bs_lexer_buffer(l, bs_lexer_next(l));
    }
    return l->buffer;
}

bool bs_lexer_read(Bs_Lexer *l, Bs_Token_Type type) {
    bs_lexer_peek(l);
    l->peeked = l->buffer.type != type;
    return !l->peeked;
}

Bs_Token bs_lexer_expect(Bs_Lexer *l, Bs_Token_Type type) {
    const Bs_Token token = bs_lexer_next(l);
    if (token.type != type) {
        bs_fmt(
            l->error,
            Bs_Loc_Fmt "error: expected %s, got %s\n",
            Bs_Loc_Arg(token.loc),
            bs_token_type_name(type, l->extended),
            bs_token_type_name(token.type, l->extended));

        bs_lexer_error(l);
    }
    return token;
}

Bs_Token bs_lexer_either(Bs_Lexer *l, Bs_Token_Type a, Bs_Token_Type b) {
    const Bs_Token token = bs_lexer_next(l);
    if (token.type != a && token.type != b) {
        bs_fmt(
            l->error,
            Bs_Loc_Fmt "error: expected %s or %s, got %s\n",
            Bs_Loc_Arg(token.loc),
            bs_token_type_name(a, l->extended),
            bs_token_type_name(b, l->extended),
            bs_token_type_name(token.type, l->extended));

        bs_lexer_error(l);
    }
    return token;
}
