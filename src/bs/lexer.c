#include <assert.h>
#include <ctype.h>
#include <string.h>

#include "bs/lexer.h"

void bs_error_write_default(Bs_Error_Writer *w, Bs_Error error) {
    if (error.native) {
        fprintf(stderr, "[C]: ");
    } else if (error.type != BS_ERROR_STANDALONE) {
        fprintf(stderr, Bs_Loc_Fmt, Bs_Loc_Arg(error.loc));
    }

    if (error.type == BS_ERROR_TRACE) {
        fprintf(stderr, "in ");
    } else if (error.type != BS_ERROR_PANIC) {
        fprintf(stderr, "error: ");
    }

    fprintf(stderr, Bs_Sv_Fmt "\n", Bs_Sv_Arg(error.message));

    if (!error.native && error.type != BS_ERROR_STANDALONE) {
        const Bs_Sv line = bs_sv_trim(error.loc.line, '\r'); // Video Game OS fix
        fprintf(stderr, "\n    %zu | " Bs_Sv_Fmt "\n", error.loc.row, Bs_Sv_Arg(line));

        const int count = snprintf(NULL, 0, "    %zu", error.loc.row);
        assert(count >= 0);
        for (size_t i = 0; i < count; i++) {
            fputc(' ', stderr);
        }
        fputs(" | ", stderr);

        for (size_t i = 0; i + 1 < error.loc.col; i++) {
            fputc(line.data[i] == '\t' ? '\t' : ' ', stderr);
        }
        fputs("^\n", stderr);
    }

    if (!error.explanation.size && !error.example.size && error.continued) {
        fputc('\n', stderr);
    }

    if (error.explanation.size) {
        fprintf(stderr, "\n" Bs_Sv_Fmt "\n", Bs_Sv_Arg(error.explanation));
    }

    if (error.example.size) {
        fprintf(stderr, "\n```\n" Bs_Sv_Fmt "\n```\n", Bs_Sv_Arg(error.example));
    }

    if ((error.explanation.size || error.example.size) && error.continued) {
        fprintf(stderr, "\n");
    }
}

static bool ishex(char c) {
    return isdigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

static Bs_Sv line_from_sv(Bs_Sv sv) {
    const char *p = memchr(sv.data, '\n', sv.size);
    sv.size = p ? p - sv.data : sv.size;
    return sv;
}

void bs_lexer_advance(Bs_Lexer *l) {
    if (*l->sv.data == '\n') {
        if (l->sv.size > 1) {
            l->loc.row += 1;
            l->loc.col = 1;

            l->sv.data += 1;
            l->sv.size -= 1;

            l->loc.line = line_from_sv(l->sv);
            return;
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

Bs_Lexer bs_lexer_new(Bs_Sv path, Bs_Sv input, Bs_Error_Writer *error) {
    return (Bs_Lexer){
        .sv = input,
        .loc.path = path,
        .loc.row = 1,
        .loc.col = 1,
        .loc.line = line_from_sv(input),
        .error = error,
    };
}

void bs_lexer_buffer(Bs_Lexer *l, Bs_Token token) {
    l->peeked = true;
    l->buffer = token;
}

Bs_Token bs_lexer_str(Bs_Lexer *l, Bs_Loc loc, char end) {
    Bs_Token token = {.sv = l->sv, .loc = loc};

    while (l->sv.size > 0 && *l->sv.data != end) {
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
            case '\'':
            case '"':
            case '\\':
                break;

            case '(':
                token.type = BS_TOKEN_ISTR;
                token.sv.size -= l->sv.size;

                bs_lexer_advance(l);
                return token;

            default:
                bs_lexer_error(l, l->loc, "invalid escape character");
            }

            bs_lexer_advance(l);
        }

        bs_lexer_advance(l);
    }

    if (!l->sv.size) {
        bs_lexer_error(l, l->loc, "unterminated string");
    }

    token.type = BS_TOKEN_STR;
    token.sv.size -= l->sv.size;

    bs_lexer_advance(l);
    return token;
}

static_assert(BS_COUNT_TOKENS == 76, "Update bs_lexer_next()");
Bs_Token bs_lexer_next(Bs_Lexer *l) {
    if (l->peeked) {
        l->peeked = false;
        l->prev_row = l->buffer.loc.row;
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
                l->prev_row = token.loc.row;
                return token;
            }
        } else if (l->sv.data[0] == '/' && l->sv.size > 1 && l->sv.data[1] == '#') {
            if (l->comments) {
                token.sv = l->sv;
                token.loc = l->loc;
            }

            size_t depth = 0;
            while (l->sv.size >= 2) {
                if (l->sv.data[0] == '/' && l->sv.data[1] == '#') {
                    depth++;
                } else if (l->sv.data[0] == '#' && l->sv.data[1] == '/') {
                    depth--;
                    if (!depth) {
                        break;
                    }
                }

                bs_lexer_advance(l);
            }

            if (depth) {
                bs_lexer_error(l, l->loc, "unterminated comment");
            }

            // Skip the '#}'
            bs_lexer_advance(l);
            bs_lexer_advance(l);

            if (l->comments) {
                token.type = BS_TOKEN_COMMENT;
                token.sv.size -= l->sv.size;
                l->prev_row = token.loc.row;
                return token;
            }
        } else {
            break;
        }
    }

    token.sv = l->sv;
    token.loc = l->loc;
    if (l->sv.size == 0) {
        l->prev_row = token.loc.row;
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
            bs_lexer_error(l, l->loc, "invalid digit '%c' in number", *l->sv.data);
        }

        token.sv.size -= l->sv.size;
        l->prev_row = token.loc.row;
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
            bs_lexer_error(l, l->loc, "invalid digit '%c' in number", *l->sv.data);
        }

        token.sv.size -= l->sv.size;
        l->prev_row = token.loc.row;
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
        } else if (bs_sv_eq(token.sv, Bs_Sv_Static("match"))) {
            token.type = BS_TOKEN_MATCH;
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

        l->prev_row = token.loc.row;
        return token;
    }

    if (bs_lexer_match(l, '"')) {
        l->prev_row = token.loc.row;
        return bs_lexer_str(l, token.loc, '"');
    }

    if (bs_lexer_match(l, '\'')) {
        l->prev_row = token.loc.row;
        return bs_lexer_str(l, token.loc, '\'');
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

    case '$':
        if (bs_lexer_match(l, '=')) {
            token.type = BS_TOKEN_JOIN_SET;
        } else {
            token.type = BS_TOKEN_JOIN;
        }
        break;

    case '+':
        if (bs_lexer_match(l, '=')) {
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
        bs_lexer_error(l, token.loc, "invalid character '%c' (%d)", *token.sv.data, *token.sv.data);
    }

    token.sv.size -= l->sv.size;
    l->prev_row = token.loc.row;
    return token;
}

Bs_Token bs_lexer_peek(Bs_Lexer *l) {
    const size_t prev_row = l->prev_row;
    if (!l->peeked) {
        bs_lexer_buffer(l, bs_lexer_next(l));
        l->prev_row = prev_row;
    }
    return l->buffer;
}

bool bs_lexer_peek_row(Bs_Lexer *l, Bs_Token *token) {
    *token = bs_lexer_peek(l);
    return token->loc.row == l->prev_row;
}

bool bs_lexer_read(Bs_Lexer *l, Bs_Token_Type type) {
    bs_lexer_peek(l);
    l->peeked = l->buffer.type != type;
    return !l->peeked;
}

Bs_Token bs_lexer_expect(Bs_Lexer *l, Bs_Token_Type type) {
    const Bs_Token token = bs_lexer_next(l);
    if (token.type != type) {
        bs_lexer_error(
            l,
            token.loc,
            "expected %s, got %s",
            bs_token_type_name(type),
            bs_token_type_name(token.type));
    }
    return token;
}

Bs_Token bs_lexer_either(Bs_Lexer *l, Bs_Token_Type a, Bs_Token_Type b) {
    const Bs_Token token = bs_lexer_next(l);
    if (token.type != a && token.type != b) {
        bs_lexer_error(
            l,
            token.loc,
            "expected %s or %s, got %s",
            bs_token_type_name(a),
            bs_token_type_name(b),
            bs_token_type_name(token.type));
    }
    return token;
}

void bs_lexer_error_full(
    Bs_Lexer *l, Bs_Loc loc, Bs_Sv explanation, Bs_Sv example, const char *fmt, ...) {
    Bs_Error error = {0};
    error.loc = loc;
    error.type = BS_ERROR_MAIN;

    va_list args;
    va_start(args, fmt);
    static char buffer[1024];
    const int count = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    assert(count >= 0 && count + 1 < sizeof(buffer));
    error.message = Bs_Sv(buffer, count);
    error.explanation = explanation;
    error.example = example;
    error.continued = l->is_meta;

    l->error->write(l->error, error);
    longjmp(l->unwind, 1);
}
