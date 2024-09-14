#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "bs/lexer.h"

static void bsdoc_writer(Bs_Writer *w, Bs_Sv sv) {
    fwrite(sv.data, sv.size, 1, w->data);
}

typedef enum {
    STYLE_NORMAL,
    STYLE_STRING,
    STYLE_COMMENT,
    STYLE_KEYWORD,
    STYLE_CONSTANT
} Bsdoc_Style;

static Bsdoc_Style bsdoc_token_type_style(Bs_Token_Type type, bool extended) {
    switch (type) {
    case BS_TOKEN_STR:
    case BS_TOKEN_ISTR:
        return STYLE_STRING;

    case BS_TOKEN_COMMENT:
        return STYLE_COMMENT;

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
        return STYLE_KEYWORD;

    case BS_TOKEN_EOL:
    case BS_TOKEN_LNOT:
    case BS_TOKEN_SET:
        return extended ? STYLE_KEYWORD : STYLE_NORMAL;

    case BS_TOKEN_NIL:
    case BS_TOKEN_NUM:
    case BS_TOKEN_TRUE:
    case BS_TOKEN_FALSE:
    case BS_TOKEN_IS_MAIN_MODULE:
        return STYLE_CONSTANT;

    default:
        return STYLE_NORMAL;
    }
}

static void bsdoc_print_styled_sv(Bsdoc_Style style, Bs_Sv sv) {
    switch (style) {
    case STYLE_NORMAL:
        break;

    case STYLE_STRING:
        printf("<span class='string'>");
        break;

    case STYLE_COMMENT:
        printf("<span class='comment'>");
        break;

    case STYLE_KEYWORD:
        printf("<span class='keyword'>");
        break;

    case STYLE_CONSTANT:
        printf("<span class='constant'>");
        break;
    }

    fwrite(sv.data, sv.size, 1, stdout);

    if (style != STYLE_NORMAL) {
        printf("</span>");
    }
}

static void bsdoc_print_token(Bs_Token token, const char **last, bool extended) {
    if (token.type == BS_TOKEN_STR || token.type == BS_TOKEN_ISTR) {
        if (token.sv.data[-1] == '"') {
            token.sv.data--;
            token.sv.size++;
        }

        if (token.sv.data[token.sv.size] == '"') {
            token.sv.size++;
        }
    }

    fwrite(*last, token.sv.data - *last, 1, stdout);
    bsdoc_print_styled_sv(bsdoc_token_type_style(token.type, extended), token.sv);

    *last = token.sv.data + token.sv.size;
}

typedef struct {
    bool data[256];
    size_t count;
} Bsdoc_Parens;

static void bsdoc_parens_push(Bsdoc_Parens *f, bool start) {
    assert(f->count < bs_c_array_size(f->data));
    f->data[f->count++] = start;
}

static bool bsdoc_print_code(const char *path, Bs_Sv contents, size_t start, bool extended) {
    Bsdoc_Parens parens = {0};
    Bs_Writer error = {stderr, bsdoc_writer};

    Bs_Lexer lexer = bs_lexer_new(path, contents, &error);
    lexer.loc.row = start;
    lexer.comments = true;
    lexer.extended = extended;

    if (setjmp(lexer.unwind)) {
        return false;
    }

    const char *last = contents.data;
    while (lexer.sv.size) {
        Bs_Token token = bs_lexer_next(&lexer);
        bsdoc_print_token(token, &last, extended);

        switch (token.type) {
        case BS_TOKEN_ISTR:
            token = bs_lexer_expect(&lexer, BS_TOKEN_LPAREN);
            bsdoc_print_token(token, &last, extended);
            bsdoc_parens_push(&parens, true);
            break;

        case BS_TOKEN_LPAREN:
            bsdoc_parens_push(&parens, false);
            break;

        case BS_TOKEN_RPAREN:
            assert(parens.count);
            if (parens.data[--parens.count]) {
                token = bs_lexer_str(&lexer, lexer.loc);
                bsdoc_print_token(token, &last, extended);
            }
            break;

        default:
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

int main(int argc, char **argv) {
    int result = 0;

    if (argc < 2) {
        fprintf(stderr, "error: file path not provided\n");
        fprintf(stderr, "usage: %s <path>\n", *argv);
        exit(1);
    }
    const char *path = argv[1];

    size_t size;
    char *contents = bs_read_file(path, &size);
    if (!contents) {
        fprintf(stderr, "error: could not read file '%s'\n", path);
        exit(1);
    }

    printf("<!doctype html>\n"
           "<html>\n"
           "<head>\n"
           "<link rel='stylesheet' href='style.css'>\n"
           "<meta name='viewport' content='width=device-width, initial-scale=1.0'>\n"
           "<script src='script.js'></script>\n"
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
                    path,
                    row);
                bs_return_defer(1);
            }

            line.data += level;
            line.size -= level;
            printf("<h%zu>" Bs_Sv_Fmt "</h%zu>\n", level, Bs_Sv_Arg(line), level);
            continue;
        }

        if (bs_sv_eq(line, Bs_Sv_Static("```"))) {
            size_t start = row + 1;

            printf(
                "<div>\n"
                "<div class='tabs'>\n"
                "<button class='tab active' onclick='tabClick(this)'>BS</button>\n"
                "<button class='tab' onclick='tabClick(this)'>BSX</button>\n"
                "</div>\n"
                "<div class='codes'>\n"

                // Shamelessly copied from Github
                "<button class='copy' onclick='copyClick(this)'>\n"
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
                "</button>\n");

            printf("<pre class='code active'>\n");
            if (!bsdoc_print_code(
                    path, bsdoc_split_code(&sv, &row, Bs_Sv_Static("---")), start, false)) {
                bs_return_defer(1);
            }
            printf("</pre>\n");

            start = row + 1;

            printf("<pre class='code'>\n");
            if (!bsdoc_print_code(
                    path, bsdoc_split_code(&sv, &row, Bs_Sv_Static("```")), start, true)) {
                bs_return_defer(1);
            }
            printf("</pre>\n"
                   "</div>\n"
                   "</div>\n");

            continue;
        }

        printf("<p>\n" Bs_Sv_Fmt "\n", Bs_Sv_Arg(line));
        while (sv.size) {
            row++;

            line = bs_sv_split(&sv, '\n');
            if (!line.size) {
                break;
            }
            printf(Bs_Sv_Fmt "\n", Bs_Sv_Arg(line));
        }
        printf("</p>\n");
    }

    printf("</body>\n"
           "</html>\n");

defer:
    free(contents);
    return result;
}
