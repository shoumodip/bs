#include <assert.h>
#include <ctype.h>

#include "bs/lexer.h"

static bool ishex(char c) {
    return isdigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

void bs_lexer_advance(Bs_Lexer *l) {
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

Bs_Lexer bs_lexer_new(Bs_Sv path, Bs_Sv input, Bs_Writer *error) {
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
            case 'e':
            case 'n':
            case 'r':
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

static_assert(BS_COUNT_TOKENS == 75, "Update bs_lexer_next()");
Bs_Token bs_lexer_next(Bs_Lexer *l) {
    if (l->peeked) {
        l->peeked = false;
        return l->buffer;
    }

    Bs_Token token = {0};
    while (l->sv.size > 0) {
        if (isspace(*l->sv.data)) {
            bs_lexer_advance(l);
        } else if (*l->sv.data == '#') {
            if (l->comments) {
                token.sv = l->sv;
                token.loc = l->loc;
            }

            while (l->sv.size > 0 && *l->sv.data != '\n') {
                bs_lexer_advance(l);
            }

            if (l->comments) {
                token.type = BS_TOKEN_COMMENT;
                token.sv.size -= l->sv.size;
                return token;
            }
        } else {
            break;
        }
    }

    token.sv = l->sv;
    token.loc = l->loc;
    if (l->sv.size == 0) {
        return token;
    }

    if (bs_sv_prefix(l->sv, Bs_Sv_Static("0x"))) {
        token.type = BS_TOKEN_NUM;
        bs_lexer_advance(l);
        bs_lexer_advance(l);

        while (l->sv.size > 0 && ishex(*l->sv.data)) {
            bs_lexer_advance(l);
        }

        if (isalpha(*l->sv.data) || *l->sv.data == '_') {
            bs_fmt(
                l->error,
                Bs_Loc_Fmt "error: invalid digit '%c' in number\n",
                Bs_Loc_Arg(l->loc),
                *l->sv.data);

            bs_lexer_error(l);
        }

        token.sv.size -= l->sv.size;
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

        if (isalpha(*l->sv.data) || *l->sv.data == '_') {
            bs_fmt(
                l->error,
                Bs_Loc_Fmt "error: invalid digit '%c' in number\n",
                Bs_Loc_Arg(l->loc),
                *l->sv.data);

            bs_lexer_error(l);
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

        if (bs_sv_eq(token.sv, Bs_Sv_Static("nil"))) {
            token.type = BS_TOKEN_NIL;
        } else if (bs_sv_eq(token.sv, Bs_Sv_Static("true"))) {
            token.type = BS_TOKEN_TRUE;
        } else if (bs_sv_eq(token.sv, Bs_Sv_Static("false"))) {
            token.type = BS_TOKEN_FALSE;
        } else if (bs_sv_eq(token.sv, Bs_Sv_Static("len"))) {
            token.type = BS_TOKEN_LEN;
        } else if (bs_sv_eq(token.sv, Bs_Sv_Static("panic"))) {
            token.type = BS_TOKEN_PANIC;
        } else if (bs_sv_eq(token.sv, Bs_Sv_Static("assert"))) {
            token.type = BS_TOKEN_ASSERT;
        } else if (bs_sv_eq(token.sv, Bs_Sv_Static("delete"))) {
            token.type = BS_TOKEN_DELETE;
        } else if (bs_sv_eq(token.sv, Bs_Sv_Static("import"))) {
            token.type = BS_TOKEN_IMPORT;
        } else if (bs_sv_eq(token.sv, Bs_Sv_Static("typeof"))) {
            token.type = BS_TOKEN_TYPEOF;
        } else if (bs_sv_eq(token.sv, Bs_Sv_Static("if"))) {
            token.type = BS_TOKEN_IF;
        } else if (bs_sv_eq(token.sv, Bs_Sv_Static("then"))) {
            token.type = BS_TOKEN_THEN;
        } else if (bs_sv_eq(token.sv, Bs_Sv_Static("else"))) {
            token.type = BS_TOKEN_ELSE;
        } else if (bs_sv_eq(token.sv, Bs_Sv_Static("in"))) {
            token.type = BS_TOKEN_IN;
        } else if (bs_sv_eq(token.sv, Bs_Sv_Static("is"))) {
            token.type = BS_TOKEN_IS;
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
        } else if (bs_sv_eq(token.sv, Bs_Sv_Static("this"))) {
            token.type = BS_TOKEN_THIS;
        } else if (bs_sv_eq(token.sv, Bs_Sv_Static("super"))) {
            token.type = BS_TOKEN_SUPER;
        } else if (bs_sv_eq(token.sv, Bs_Sv_Static("class"))) {
            token.type = BS_TOKEN_CLASS;
        } else if (bs_sv_eq(token.sv, Bs_Sv_Static("is_main_module"))) {
            token.type = BS_TOKEN_IS_MAIN_MODULE;
        }

        return token;
    }

    if (bs_lexer_match(l, '"')) {
        return bs_lexer_str(l, token.loc);
    }

    switch (bs_lexer_consume(l)) {
    case ';':
        token.type = BS_TOKEN_EOL;
        break;

    case '.':
        token.type = BS_TOKEN_DOT;
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
        if (bs_lexer_match(l, '+')) {
            if (bs_lexer_match(l, '=')) {
                token.type = BS_TOKEN_JOIN_SET;
            } else {
                token.type = BS_TOKEN_JOIN;
            }
        } else if (bs_lexer_match(l, '=')) {
            token.type = BS_TOKEN_ADD_SET;
        } else {
            token.type = BS_TOKEN_ADD;
        }
        break;

    case '-':
        if (bs_lexer_match(l, '=')) {
            token.type = BS_TOKEN_SUB_SET;
        } else {
            token.type = BS_TOKEN_SUB;
        }
        break;

    case '*':
        if (bs_lexer_match(l, '=')) {
            token.type = BS_TOKEN_MUL_SET;
        } else {
            token.type = BS_TOKEN_MUL;
        }
        break;

    case '/':
        if (bs_lexer_match(l, '=')) {
            token.type = BS_TOKEN_DIV_SET;
        } else {
            token.type = BS_TOKEN_DIV;
        }
        break;

    case '%':
        if (bs_lexer_match(l, '=')) {
            token.type = BS_TOKEN_MOD_SET;
        } else {
            token.type = BS_TOKEN_MOD;
        }
        break;

    case '|':
        if (bs_lexer_match(l, '|')) {
            token.type = BS_TOKEN_LOR;
        } else if (bs_lexer_match(l, '=')) {
            token.type = BS_TOKEN_BOR_SET;
        } else {
            token.type = BS_TOKEN_BOR;
        }
        break;

    case '&':
        if (bs_lexer_match(l, '&')) {
            token.type = BS_TOKEN_LAND;
        } else if (bs_lexer_match(l, '=')) {
            token.type = BS_TOKEN_BAND_SET;
        } else {
            token.type = BS_TOKEN_BAND;
        }
        break;

    case '^':
        if (bs_lexer_match(l, '=')) {
            token.type = BS_TOKEN_BXOR_SET;
        } else {
            token.type = BS_TOKEN_BXOR;
        }
        break;

    case '~':
        token.type = BS_TOKEN_BNOT;
        break;

    case '!':
        if (bs_lexer_match(l, '=')) {
            token.type = BS_TOKEN_NE;
        } else {
            token.type = BS_TOKEN_LNOT;
        }
        break;

    case '>':
        if (bs_lexer_match(l, '>')) {
            if (bs_lexer_match(l, '=')) {
                token.type = BS_TOKEN_SHR_SET;
            } else {
                token.type = BS_TOKEN_SHR;
            }
        } else if (bs_lexer_match(l, '=')) {
            token.type = BS_TOKEN_GE;
        } else {
            token.type = BS_TOKEN_GT;
        }
        break;

    case '<':
        if (bs_lexer_match(l, '<')) {
            if (bs_lexer_match(l, '=')) {
                token.type = BS_TOKEN_SHL_SET;
            } else {
                token.type = BS_TOKEN_SHL;
            }
        } else if (bs_lexer_match(l, '=')) {
            token.type = BS_TOKEN_LE;
        } else {
            token.type = BS_TOKEN_LT;
        }
        break;

    case '=':
        if (bs_lexer_match(l, '=')) {
            token.type = BS_TOKEN_EQ;
        } else if (bs_lexer_match(l, '>')) {
            token.type = BS_TOKEN_ARROW;
        } else {
            token.type = BS_TOKEN_SET;
        }
        break;
    }

    if (token.type == BS_TOKEN_EOF) {
        bs_fmt(
            l->error,
            Bs_Loc_Fmt "error: invalid character '%c' (%d)\n",
            Bs_Loc_Arg(token.loc),
            *token.sv.data,
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
            bs_token_type_name(type),
            bs_token_type_name(token.type));

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
            bs_token_type_name(a),
            bs_token_type_name(b),
            bs_token_type_name(token.type));

        bs_lexer_error(l);
    }
    return token;
}
