#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bs/lexer.h"

static void bsdoc_writer(Bs_Writer *w, Bs_Sv sv) {
    fwrite(sv.data, sv.size, 1, w->data);
}

typedef enum {
    BSDOC_STYLE_NONE,
    BSDOC_STYLE_FIELD,
    BSDOC_STYLE_ESCAPE,
    BSDOC_STYLE_STRING,
    BSDOC_STYLE_COMMENT,
    BSDOC_STYLE_KEYWORD,
    BSDOC_STYLE_CONSTANT,
    BSDOC_STYLE_FUNCTION,
} Bsdoc_Style;

static Bsdoc_Style bsdoc_token_type_style(Bs_Token_Type type, bool extended) {
    switch (type) {
    case BS_TOKEN_STR:
    case BS_TOKEN_ISTR:
        return BSDOC_STYLE_STRING;

    case BS_TOKEN_COMMENT:
        return BSDOC_STYLE_COMMENT;

    case BS_TOKEN_LEN:
    case BS_TOKEN_IMPORT:
    case BS_TOKEN_TYPEOF:
    case BS_TOKEN_IF:
    case BS_TOKEN_THEN:
    case BS_TOKEN_ELSE:
    case BS_TOKEN_IN:
    case BS_TOKEN_FOR:
    case BS_TOKEN_WHILE:
    case BS_TOKEN_BREAK:
    case BS_TOKEN_CONTINUE:
    case BS_TOKEN_FN:
    case BS_TOKEN_PUB:
    case BS_TOKEN_VAR:
    case BS_TOKEN_RETURN:
        return BSDOC_STYLE_KEYWORD;

    case BS_TOKEN_EOL:
    case BS_TOKEN_LNOT:
        return extended ? BSDOC_STYLE_KEYWORD : BSDOC_STYLE_NONE;

    case BS_TOKEN_NIL:
    case BS_TOKEN_NUM:
    case BS_TOKEN_TRUE:
    case BS_TOKEN_FALSE:
    case BS_TOKEN_IS_MAIN_MODULE:
        return BSDOC_STYLE_CONSTANT;

    default:
        return BSDOC_STYLE_NONE;
    }
}

static void bsdoc_print_styled_sv(FILE *f, Bsdoc_Style style, Bs_Sv sv) {
    switch (style) {
    case BSDOC_STYLE_NONE:
        break;

    case BSDOC_STYLE_FIELD:
        fprintf(f, "<span class='field'>");
        break;

    case BSDOC_STYLE_ESCAPE:
        fprintf(f, "<span class='escape'>");
        break;

    case BSDOC_STYLE_STRING:
        fprintf(f, "<span class='string'>");
        break;

    case BSDOC_STYLE_COMMENT:
        fprintf(f, "<span class='comment'>");
        break;

    case BSDOC_STYLE_KEYWORD:
        fprintf(f, "<span class='keyword'>");
        break;

    case BSDOC_STYLE_CONSTANT:
        fprintf(f, "<span class='constant'>");
        break;

    case BSDOC_STYLE_FUNCTION:
        fprintf(f, "<span class='function'>");
        break;
    }

    fwrite(sv.data, sv.size, 1, f);

    if (style != BSDOC_STYLE_NONE) {
        fprintf(f, "</span>");
    }
}

static void bsdoc_print_token(FILE *f, Bs_Token token, Bsdoc_Style style, const char **last) {
    const bool string = token.type == BS_TOKEN_STR || token.type == BS_TOKEN_ISTR;
    bool interpolation = false;
    if (string) {
        if (token.sv.data[-1] == '"') {
            token.sv.data--;
            token.sv.size++;
        }

        if (token.sv.data[token.sv.size] == '"') {
            token.sv.size++;
        } else if (token.sv.data[token.sv.size] == '\\') {
            interpolation = true;
        }
    }

    fwrite(*last, token.sv.data - *last, 1, f);
    *last = token.sv.data + token.sv.size;

    if (string) {
        while (token.sv.size) {
            size_t index;
            if (bs_sv_find(token.sv, '\\', &index)) {
                bsdoc_print_styled_sv(f, style, bs_sv_drop(&token.sv, index));
                bsdoc_print_styled_sv(f, BSDOC_STYLE_ESCAPE, bs_sv_drop(&token.sv, 2));
            } else {
                bsdoc_print_styled_sv(f, style, bs_sv_drop(&token.sv, token.sv.size));
            }
        }
    } else {
        bsdoc_print_styled_sv(f, style, token.sv);
    }

    if (interpolation) {
        bsdoc_print_styled_sv(f, BSDOC_STYLE_ESCAPE, Bs_Sv_Static("\\"));
        (*last)++;
    }
}

typedef struct {
    bool data[256];
    size_t count;
} Bsdoc_Parens;

static void bsdoc_parens_push(Bsdoc_Parens *f, bool start) {
    assert(f->count < bs_c_array_size(f->data));
    f->data[f->count++] = start;
}

static bool bsdoc_print_code(FILE *f, const char *path, Bs_Sv input, size_t start, bool extended) {
    Bsdoc_Parens parens = {0};
    Bs_Writer error = {stderr, bsdoc_writer};

    Bs_Lexer lexer = bs_lexer_new(bs_sv_from_cstr(path), input, &error);
    lexer.loc.row = start;
    lexer.comments = true;
    lexer.extended = extended;

    if (setjmp(lexer.unwind)) {
        return false;
    }

    Bsdoc_Style next = BSDOC_STYLE_NONE;
    const char *last = input.data;
    while (lexer.sv.size) {
        Bs_Token token = bs_lexer_next(&lexer);
        if (next != BSDOC_STYLE_NONE && token.type != BS_TOKEN_IDENT &&
            token.type != BS_TOKEN_COMMENT) {
            next = BSDOC_STYLE_NONE;
        }

        Bsdoc_Style style;
        if (token.type == BS_TOKEN_IDENT && next != BSDOC_STYLE_NONE) {
            style = next;
            next = BSDOC_STYLE_NONE;
        } else {
            style = bsdoc_token_type_style(token.type, extended);
        }

        switch (token.type) {
        case BS_TOKEN_ISTR:
            bsdoc_print_token(f, token, style, &last);

            token = bs_lexer_expect(&lexer, BS_TOKEN_LPAREN);
            bsdoc_print_token(f, token, BSDOC_STYLE_ESCAPE, &last);

            bsdoc_parens_push(&parens, true);
            break;

        case BS_TOKEN_LPAREN:
            bsdoc_print_token(f, token, style, &last);
            bsdoc_parens_push(&parens, false);
            break;

        case BS_TOKEN_RPAREN:
            assert(parens.count);
            if (parens.data[--parens.count]) {
                bsdoc_print_token(f, token, BSDOC_STYLE_ESCAPE, &last);

                token = bs_lexer_str(&lexer, lexer.loc);
                bsdoc_print_token(f, token, bsdoc_token_type_style(token.type, extended), &last);

                if (token.type == BS_TOKEN_ISTR) {
                    token = bs_lexer_expect(&lexer, BS_TOKEN_LPAREN);
                    bsdoc_print_token(
                        f, token, bsdoc_token_type_style(token.type, extended), &last);
                    bsdoc_parens_push(&parens, true);
                }
            } else {
                bsdoc_print_token(f, token, style, &last);
            }
            break;

        case BS_TOKEN_FN:
            bsdoc_print_token(f, token, style, &last);
            next = BSDOC_STYLE_FUNCTION;
            break;

        case BS_TOKEN_DOT:
            bsdoc_print_token(f, token, style, &last);
            next = BSDOC_STYLE_FIELD;
            break;

        default:
            bsdoc_print_token(f, token, style, &last);
            break;
        }
    }

    return true;
}

static Bs_Sv bsdoc_split_code(Bs_Sv *sv, size_t *row, Bs_Sv end) {
    Bs_Sv block = Bs_Sv(sv->data, 0);
    while (sv->size) {
        (*row)++;

        const Bs_Sv line = bs_sv_split(sv, '\n');
        if (bs_sv_eq(line, end)) {
            break;
        }

        block.size += line.size + 1;
    }
    return block;
}

// Shamelessly copied from Github
static void bsdoc_print_copy(FILE *f, const char *onclick) {
    fprintf(
        f,
        "<button class='copy' onclick='%s(this)'>\n"
        "<svg height='16' viewBox='0 0 16 16' version='1.1' width='16'>\n"
        "    <path d='M0 6.75C0 5.784.784 5 1.75 5h1.5a.75.75 0 0 1 0 1.5h-1.5a.25.25 0 0 "
        "0-.25.25v7.5c0 .138.112.25.25.25h7.5a.25.25 0 0 0 .25-.25v-1.5a.75.75 0 0 1 1.5 "
        "0v1.5A1.75 1.75 0 0 1 9.25 16h-7.5A1.75 1.75 0 0 1 0 14.25Z'></path><path d='M5 "
        "1.75C5 .784 5.784 0 6.75 0h7.5C15.216 0 16 .784 16 1.75v7.5A1.75 1.75 0 0 1 14.25 "
        "11h-7.5A1.75 1.75 0 0 1 5 9.25Zm1.75-.25a.25.25 0 0 0-.25.25v7.5c0 "
        ".138.112.25.25.25h7.5a.25.25 0 0 0 .25-.25v-7.5a.25.25 0 0 0-.25-.25Z'></path>\n"
        "</svg>\n"
        "</button>\n"
        "<button class='copied hidden'>\n"
        "<svg height='16' viewBox='0 0 16 16' version='1.1' width='16'>\n"
        "    <path d='M13.78 4.22a.75.75 0 0 1 0 1.06l-7.25 7.25a.75.75 0 0 1-1.06 0L2.22 "
        "9.28a.751.751 0 0 1 .018-1.042.751.751 0 0 1 1.042-.018L6 10.94l6.72-6.72a.75.75 "
        "0 0 1 1.06 0Z'></path>\n"
        "</svg>\n"
        "</button>\n",
        onclick);
}

int main(int argc, char **argv) {
    int result = 0;

    if (argc < 2) {
        fprintf(stderr, "error: file path not provided\n");
        fprintf(stderr, "usage: %s <path>\n", *argv);
        exit(1);
    }
    const char *input = argv[1];

    size_t size;
    char *contents = bs_read_file(input, &size);
    if (!contents) {
        fprintf(stderr, "error: could not read file '%s'\n", input);
        exit(1);
    }

    char *output = NULL;
    {
        const size_t length = strlen(input) - 2;

        output = malloc(length + 5);
        assert(output);

        memcpy(output, input, length);
        memcpy(output + length, "html", 5);
    }

    FILE *f = fopen(output, "wb");
    if (!f) {
        fprintf(stderr, "error: could not write file '%s'\n", output);
        exit(1);
    }

    fprintf(
        f,
        "<!doctype html>\n"
        "<html>\n"
        "<head>\n"
        "<link rel='stylesheet' href='/docs/style.css'>\n"
        "<meta name='viewport' content='width=device-width, initial-scale=1.0'>\n"
        "<script src='/docs/script.js'></script>\n"
        "</head>\n"
        "<body>\n");

    Bs_Sv sv = Bs_Sv(contents, size);
    for (size_t row = 1; sv.size; row++) {
        Bs_Sv line = bs_sv_split(&sv, '\n');
        if (!line.size) {
            continue;
        }

        if (*line.data == '#') {
            size_t level = 0;
            while (level < line.size) {
                if (line.data[level] != '#') {
                    break;
                }

                level++;
            }

            if (level > 6) {
                fprintf(
                    stderr,
                    "%s:%zu:1: error: invalid heading depth, number of '#' must be less than 6\n",
                    input,
                    row);
                bs_return_defer(1);
            }

            line.data += level;
            line.size -= level;
            fprintf(f, "<h%zu>" Bs_Sv_Fmt "</h%zu>\n", level, Bs_Sv_Arg(line), level);
            continue;
        }

        if (*line.data == '[') {
            line.data++;
            line.size--;

            size_t index;
            const Bs_Sv name = bs_sv_split(&line, ']');

            line.data++;
            line.size -= 2;

            fprintf(
                f, "<a href='" Bs_Sv_Fmt "'>" Bs_Sv_Fmt "</a>\n", Bs_Sv_Arg(line), Bs_Sv_Arg(name));
            continue;
        }

        if (bs_sv_eq(line, Bs_Sv_Static("```"))) {
            size_t start = row + 1;

            fprintf(
                f,
                "<div>\n"
                "<div class='tabs'>\n"
                "<button class='tab active' onclick='tabClick(this)'>BS</button>\n"
                "<button class='tab' onclick='tabClick(this)'>BSX</button>\n"
                "</div>\n"
                "<div class='codes'>\n");

            bsdoc_print_copy(f, "copyClickCode");

            fprintf(f, "<pre class='code active'>\n");
            if (!bsdoc_print_code(
                    f, input, bsdoc_split_code(&sv, &row, Bs_Sv_Static("---")), start, false)) {
                bs_return_defer(1);
            }
            fprintf(f, "</pre>\n");

            start = row + 1;

            fprintf(f, "<pre class='code'>\n");
            if (!bsdoc_print_code(
                    f, input, bsdoc_split_code(&sv, &row, Bs_Sv_Static("```")), start, true)) {
                bs_return_defer(1);
            }
            fprintf(
                f,
                "</pre>\n"
                "</div>\n"
                "</div>\n");

            continue;
        }

        if (bs_sv_eq(line, Bs_Sv_Static("```console"))) {
            fprintf(f, "<div class='codes shell'>\n");
            bsdoc_print_copy(f, "copyClickShell");
            fprintf(f, "<pre class='code active shell'>\n");

            while (sv.size) {
                row++;

                line = bs_sv_split(&sv, '\n');
                if (bs_sv_eq(line, Bs_Sv_Static("```"))) {
                    break;
                }

                const Bs_Sv shell = Bs_Sv_Static("$ ");
                if (bs_sv_prefix(line, shell)) {
                    bs_sv_drop(&line, shell.size);
                    bsdoc_print_styled_sv(f, BSDOC_STYLE_CONSTANT, shell);

                    Bs_Sv comment = line;
                    line = bs_sv_split(&comment, '#');
                    fwrite(line.data, line.size, 1, f);

                    if (comment.size) {
                        comment.data--;
                        comment.size++;
                        bsdoc_print_styled_sv(f, BSDOC_STYLE_COMMENT, comment);
                    }
                } else {
                    fwrite(line.data, line.size, 1, f);
                }

                fprintf(f, "\n");
            }

            fprintf(
                f,
                "</pre>\n"
                "</div>\n");

            continue;
        }

        if (*line.data == '<') {
            fprintf(f, Bs_Sv_Fmt "\n", Bs_Sv_Arg(line));
            continue;
        }

        fprintf(f, "<p>\n" Bs_Sv_Fmt "\n", Bs_Sv_Arg(line));
        while (sv.size) {
            row++;

            line = bs_sv_split(&sv, '\n');
            if (!line.size) {
                break;
            }
            fprintf(f, Bs_Sv_Fmt "\n", Bs_Sv_Arg(line));
        }
        fprintf(f, "</p>\n");
    }

    fprintf(
        f,
        "</body>\n"
        "</html>\n");

    printf("Wrote documentation to '%s'\n", output);

defer:
    fclose(f);
    free(output);
    free(contents);
    return result;
}
