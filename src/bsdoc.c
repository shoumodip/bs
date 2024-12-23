#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bs/lexer.h"

typedef enum {
    BSDOC_STYLE_NONE,
    BSDOC_STYLE_CLASS,
    BSDOC_STYLE_FIELD,
    BSDOC_STYLE_ESCAPE,
    BSDOC_STYLE_STRING,
    BSDOC_STYLE_COMMENT,
    BSDOC_STYLE_KEYWORD,
    BSDOC_STYLE_CONSTANT,
    BSDOC_STYLE_FUNCTION,
} Bsdoc_Style;

static Bsdoc_Style bsdoc_token_type_style(Bs_Token_Type type) {
    switch (type) {
    case BS_TOKEN_STR:
    case BS_TOKEN_ISTR:
        return BSDOC_STYLE_STRING;

    case BS_TOKEN_EOL:
    case BS_TOKEN_COMMENT:
        return BSDOC_STYLE_COMMENT;

    case BS_TOKEN_LEN:
    case BS_TOKEN_PANIC:
    case BS_TOKEN_ASSERT:
    case BS_TOKEN_IMPORT:
    case BS_TOKEN_TYPEOF:
    case BS_TOKEN_DELETE:
    case BS_TOKEN_IF:
    case BS_TOKEN_THEN:
    case BS_TOKEN_ELSE:
    case BS_TOKEN_MATCH:
    case BS_TOKEN_IN:
    case BS_TOKEN_IS:
    case BS_TOKEN_FOR:
    case BS_TOKEN_WHILE:
    case BS_TOKEN_BREAK:
    case BS_TOKEN_CONTINUE:
    case BS_TOKEN_FN:
    case BS_TOKEN_PUB:
    case BS_TOKEN_VAR:
    case BS_TOKEN_CLASS:
    case BS_TOKEN_RETURN:
        return BSDOC_STYLE_KEYWORD;

    case BS_TOKEN_NIL:
    case BS_TOKEN_NUM:
    case BS_TOKEN_TRUE:
    case BS_TOKEN_FALSE:
    case BS_TOKEN_THIS:
    case BS_TOKEN_SUPER:
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

    case BSDOC_STYLE_CLASS:
        fprintf(f, "<span class='class'>");
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

    for (size_t i = 0; i < sv.size; i++) {
        const char c = sv.data[i];
        if (c == '<') {
            fputs("&lt;", f);
        } else if (c == '>') {
            fputs("&gt;", f);
        } else if (c == '&') {
            fputs("&amp;", f);
        } else {
            fputc(c, f);
        }
    }

    if (style != BSDOC_STYLE_NONE) {
        fprintf(f, "</span>");
    }
}

static void bsdoc_print_token(FILE *f, Bs_Token token, Bsdoc_Style style, const char **last) {
    const bool string = token.type == BS_TOKEN_STR || token.type == BS_TOKEN_ISTR;
    bool interpolation = false;
    if (string) {
        if (token.sv.data[-1] == '"' || token.sv.data[-1] == '\'') {
            token.sv.data--;
            token.sv.size++;
        }

        if (token.sv.data[token.sv.size] == '"' || token.sv.data[token.sv.size] == '\'') {
            token.sv.size++;
        } else if (token.sv.data[token.sv.size] == '{') {
            interpolation = true;
        }
    }

    bsdoc_print_styled_sv(f, BSDOC_STYLE_NONE, Bs_Sv(*last, token.sv.data - *last));
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
        bsdoc_print_styled_sv(f, BSDOC_STYLE_ESCAPE, Bs_Sv_Static("{"));
        (*last)++;
    }
}

typedef struct {
    char data[256];
    size_t count;
} Bsdoc_Parens;

static void bsdoc_parens_push(Bsdoc_Parens *f, char start) {
    assert(f->count < bs_c_array_size(f->data));
    f->data[f->count++] = start;
}

typedef struct {
    Bsdoc_Style style;
    Bs_Sv name;
} Bsdoc_Style_Pair;

#define p(style, name) {style, Bs_Sv_Static(name)}
static const Bsdoc_Style_Pair style_pairs[] = {
    p(BSDOC_STYLE_KEYWORD, "static"),
    p(BSDOC_STYLE_KEYWORD, "const"),
    p(BSDOC_STYLE_KEYWORD, "sizeof"),
    p(BSDOC_STYLE_CLASS, "void"),
    p(BSDOC_STYLE_CLASS, "int"),
    p(BSDOC_STYLE_CLASS, "float"),
    p(BSDOC_STYLE_CLASS, "size_t"),
    p(BSDOC_STYLE_CLASS, "Color"),
    p(BSDOC_STYLE_CLASS, "Music"),
    p(BSDOC_STYLE_CLASS, "Sound"),
    p(BSDOC_STYLE_CLASS, "Vector2"),
    p(BSDOC_STYLE_CLASS, "Texture"),
    p(BSDOC_STYLE_CLASS, "Rectangle"),
    p(BSDOC_STYLE_CLASS, "Bs"),
    p(BSDOC_STYLE_CLASS, "Bs_FFI"),
    p(BSDOC_STYLE_CLASS, "Bs_Str"),
    p(BSDOC_STYLE_CLASS, "Bs_Value"),
    p(BSDOC_STYLE_CLASS, "Bs_C_Lib"),
    p(BSDOC_STYLE_CLASS, "Bs_C_Class"),
    p(BSDOC_STYLE_CONSTANT, "BS_VALUE_NUM"),
    p(BSDOC_STYLE_CONSTANT, "BS_OBJECT_STR"),
    p(BSDOC_STYLE_CONSTANT, "BS_LIBRARY_INIT"),
    p(BSDOC_STYLE_CONSTANT, "bs_check_arity"),
    p(BSDOC_STYLE_CONSTANT, "bs_arg_check_value_type"),
    p(BSDOC_STYLE_CONSTANT, "bs_arg_check_object_type"),
    p(BSDOC_STYLE_CONSTANT, "bs_arg_check_whole_number"),
    p(BSDOC_STYLE_CONSTANT, "bs_value_nil"),
    p(BSDOC_STYLE_CONSTANT, "bs_value_num"),
    p(BSDOC_STYLE_CONSTANT, "bs_value_bool"),
    p(BSDOC_STYLE_CONSTANT, "bs_value_object"),
    p(BSDOC_STYLE_CONSTANT, "bs_c_array_size"),
    p(BSDOC_STYLE_CONSTANT, "bs_this_c_instance_data_as"),
    p(BSDOC_STYLE_CONSTANT, "Bs_Sv_Static"),
};
#undef p

static void bs_error_write(Bs_Error_Writer *w, Bs_Error error) {
    fprintf(
        stderr,
        Bs_Loc_Fmt "error: " Bs_Sv_Fmt "\n",
        Bs_Loc_Arg(error.loc),
        Bs_Sv_Arg(error.message));
}

static bool bsdoc_print_code(FILE *f, const char *path, Bs_Sv input, size_t start, bool c) {
    Bsdoc_Parens parens = {0};
    Bs_Error_Writer error = {.write = bs_error_write};

    Bs_Lexer lexer = bs_lexer_new(bs_sv_from_cstr(path), input, &error);
    lexer.loc.row = start;
    lexer.bsdoc = true;

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
        if (c) {
            if (token.type == BS_TOKEN_DOT || token.type == BS_TOKEN_ARROW) {
                next = BSDOC_STYLE_FIELD;
            }

            style = bsdoc_token_type_style(token.type);
            if (token.type == BS_TOKEN_IDENT) {
                for (size_t i = 0; i < bs_c_array_size(style_pairs); i++) {
                    const Bsdoc_Style_Pair *pair = &style_pairs[i];
                    if (bs_sv_eq(token.sv, pair->name)) {
                        style = pair->style;
                        break;
                    }
                }

                if (style == BSDOC_STYLE_NONE) {
                    if (bs_lexer_peek(&lexer).type == BS_TOKEN_LPAREN) {
                        style = BSDOC_STYLE_FUNCTION;
                    } else if (next != BSDOC_STYLE_NONE) {
                        style = next;
                        next = BSDOC_STYLE_NONE;
                    }
                }
            }

            if (token.type == BS_TOKEN_COMMENT && *token.sv.data == '#') {
                Bs_Sv macro = bs_sv_split(&token.sv, ' ');
                {
                    const Bs_Sv t = token.sv;
                    token.sv = macro;
                    macro = t;
                }

                bsdoc_print_token(f, token, BSDOC_STYLE_KEYWORD, &last);
                token.sv = macro;
                bsdoc_print_token(f, token, BSDOC_STYLE_STRING, &last);
            } else {
                bsdoc_print_token(f, token, style, &last);
            }
            continue;
        }

        if (token.type == BS_TOKEN_IDENT) {
            if (bs_lexer_peek(&lexer).type == BS_TOKEN_LPAREN) {
                style = BSDOC_STYLE_FUNCTION;
            } else if (next != BSDOC_STYLE_NONE) {
                style = next;
                next = BSDOC_STYLE_NONE;
            } else {
                style = bsdoc_token_type_style(token.type);
            }
        } else {
            style = bsdoc_token_type_style(token.type);
        }

        switch (token.type) {
        case BS_TOKEN_ISTR:
            bsdoc_print_token(f, token, style, &last);
            bsdoc_parens_push(&parens, token.sv.data[-1]);
            break;

        case BS_TOKEN_LBRACE:
            bsdoc_print_token(f, token, style, &last);
            bsdoc_parens_push(&parens, '\0');
            break;

        case BS_TOKEN_RBRACE: {
            assert(parens.count);
            const char ch = parens.data[--parens.count];
            assert(ch == '"' || ch == '\'' || ch == '\0');
            if (ch) {
                bsdoc_print_token(f, token, BSDOC_STYLE_ESCAPE, &last);

                token = bs_lexer_str(&lexer, lexer.loc, ch);
                bsdoc_print_token(f, token, bsdoc_token_type_style(token.type), &last);

                if (token.type == BS_TOKEN_ISTR) {
                    bsdoc_parens_push(&parens, ch);
                }
            } else {
                bsdoc_print_token(f, token, style, &last);
            }
        } break;

        case BS_TOKEN_FN:
            bsdoc_print_token(f, token, style, &last);
            break;

        case BS_TOKEN_CLASS:
            bsdoc_print_token(f, token, style, &last);
            next = BSDOC_STYLE_CLASS;
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

void bsdoc_print_line(FILE *f, Bs_Sv line, bool newline, bool list) {
    if (list) {
        fputs("<li>", f);
        line.data += 2;
        line.size -= 2;
    }

    bool code = false;
    for (size_t i = 0; i < line.size; i++) {
        const char c = line.data[i];
        if (c == '`') {
            code = !code;
            fputs(code ? "<code>" : "</code>", f);
        } else if (c == '<') {
            fputs("&lt;", f);
        } else if (c == '>') {
            fputs("&gt;", f);
        } else if (c == '&') {
            fputs("&amp;", f);
        } else {
            fputc(c, f);
        }
    }

    if (list) {
        fputs("</li>", f);
    }

    if (newline) {
        fputc('\n', f);
    }
}

typedef struct {
    Bs_Sv title;
    size_t level;
} Bsdoc_Section;

typedef struct {
    Bsdoc_Section data[256];
    size_t count;
} Bsdoc_Sections;

void bsdoc_sections_push(Bsdoc_Sections *s, Bsdoc_Section section) {
    assert(s->count < bs_c_array_size(s->data));
    s->data[s->count++] = section;
}

void bsdoc_print_navigator(FILE *f, Bsdoc_Sections *sections, size_t *i) {
    const size_t index = (*i)++;
    const Bsdoc_Section *s = &sections->data[index];

    fprintf(f, "<li>\n");
    fprintf(f, "<a href='#s%zu'>", index);
    bsdoc_print_line(f, s->title, false, false);
    fprintf(f, "</a>\n");

    if (*i < sections->count && sections->data[*i].level > s->level) {
        fprintf(f, "<ul>\n");
        while (*i < sections->count && sections->data[*i].level > s->level) {
            bsdoc_print_navigator(f, sections, i);
        }
        fprintf(f, "</ul>\n");
    }

    fprintf(f, "</li>\n");
}

void bsdoc_error_row(Bs_Writer *w, const char *path, size_t row, const char *message) {
    bs_fmt(w, "%s:%zu:1: ", path, row);
    bs_fmt(w, "error: %s", message);
}

int bsdoc_run_file(const char *input) {
    int result = 0;

    Bs_Writer error = bs_file_writer(stderr);

    size_t size;
    char *contents = bs_read_file(input, &size, false);
    if (!contents) {
        bs_fmt(&error, "error: could not read file '%s'\n", input);
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

    FILE *f = fopen(output, "w");
    if (!f) {
        bs_fmt(&error, "error: could not write file '%s'\n", output);
        exit(1);
    }

    fprintf(
        f,
        "<!doctype html>\n"
        "<html>\n"
        "<head>\n"
        "<meta charset='UTF-8'>\n"
        "<meta name='viewport' content='width=device-width, initial-scale=1.0'>\n"
        "<link rel='stylesheet' href='style.css'>\n"
        "</head>\n"
        "<body>\n");

    Bs_Sv sv = Bs_Sv(contents, size);
    size_t current_level = 0;

    Bsdoc_Sections sections = {0};

    Bs_Sv line = bs_sv_split(&sv, '\n');
    if (!bs_sv_prefix(line, Bs_Sv_Static("# "))) {
        bsdoc_error_row(&error, input, 1, "expected '#' on first line\n");
        bs_return_defer(1);
    }
    bs_sv_drop(&line, 2);

    fprintf(
        f,
        "<header id='header'>\n"
        "<button id='toggle-sidebar' aria-label='Toggle Sidebar'>☰</button>\n"
        "<h1>");

    bsdoc_print_line(f, line, false, false);

    fprintf(
        f,
        "</h1>\n"
        "</header>\n"
        "<main id='main'>\n");

    bool navigation = true;
    for (size_t row = 2; sv.size; row++) {
        line = bs_sv_split(&sv, '\n');
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
                bsdoc_error_row(&error, input, row, "invalid heading depth, maximum 6 allowed\n");
                bs_return_defer(1);
            }

            line.data += level;
            line.size -= level;

            for (size_t i = level; i <= current_level; i++) {
                fprintf(f, "</section>\n");
            }
            current_level = level;

            Bs_Sv highlight = line;
            line = bs_sv_split(&highlight, '@');

            Bsdoc_Section section = {
                .title = line,
                .level = level,
            };

            fprintf(f, "<section id='s%zu'>\n<h%zu", sections.count, level);
            if (highlight.size) {
                fprintf(f, " class='hl-header'>");

                line = bs_sv_trim(line, ' ');

                if (bs_sv_eq(highlight, Bs_Sv_Static("method"))) {
                    const Bs_Sv class = bs_sv_split(&line, '.');
                    section.title = bs_sv_split(&line, '(');

                    fprintf(
                        f,
                        "<code><span class='class'>" Bs_Sv_Fmt
                        "</span>.<span class='field'>" Bs_Sv_Fmt "</span>(",
                        Bs_Sv_Arg(class),
                        Bs_Sv_Arg(section.title));

                    bsdoc_print_line(f, line, false, false);
                    fprintf(f, "</code>");
                } else {
                    Bs_Sv name = line;
                    for (size_t i = 0; i < name.size; i++) {
                        if (!isalnum(name.data[i]) && name.data[i] != '_') {
                            name.size = i;
                            section.title = name;
                            break;
                        }
                    }
                    bs_sv_drop(&line, name.size);

                    fprintf(f, "<code><span class='" Bs_Sv_Fmt "'>", Bs_Sv_Arg(highlight));
                    bsdoc_print_line(f, name, false, false);
                    fprintf(f, "</span>" Bs_Sv_Fmt "</code>", Bs_Sv_Arg(line));
                }
            } else {
                fprintf(f, ">");
                bsdoc_print_line(f, line, false, false);
            }
            fprintf(f, "</h%zu>\n", level);

            bsdoc_sections_push(&sections, section);
            continue;
        }

        if (*line.data == '[') {
            line.data++;
            line.size--;

            const Bs_Sv name = bs_sv_split(&line, ']');

            line.data++;
            line.size -= 2;

            fprintf(
                f,
                "<a class='link' href='" Bs_Sv_Fmt "'>" Bs_Sv_Fmt "</a>\n",
                Bs_Sv_Arg(line),
                Bs_Sv_Arg(name));
            continue;
        }

        if (bs_sv_eq(line, Bs_Sv_Static("```bs"))) {
            size_t start = row + 1;

            fprintf(f, "<div class='codes'>\n");
            bsdoc_print_copy(f, "copyClick");
            fprintf(f, "<pre class='code active'>\n");

            if (!bsdoc_print_code(
                    f, input, bsdoc_split_code(&sv, &row, Bs_Sv_Static("```")), start, false)) {
                bs_return_defer(1);
            }

            fprintf(
                f,
                "</pre>\n"
                "</div>\n");

            continue;
        }

        if (bs_sv_eq(line, Bs_Sv_Static("```c"))) {
            size_t start = row + 1;

            fprintf(f, "<div class='codes'>\n");
            bsdoc_print_copy(f, "copyClick");
            fprintf(f, "<pre class='code active'>\n");

            if (!bsdoc_print_code(
                    f, input, bsdoc_split_code(&sv, &row, Bs_Sv_Static("```")), start, true)) {
                bs_return_defer(1);
            }

            fprintf(
                f,
                "</pre>\n"
                "</div>\n");

            continue;
        }

        if (bs_sv_eq(line, Bs_Sv_Static("```console"))) {
            fprintf(f, "<div class='codes'>\n");
            bsdoc_print_copy(f, "copyClick");
            fprintf(f, "<pre class='code active'>\n");

            while (sv.size) {
                row++;

                line = bs_sv_split(&sv, '\n');
                if (bs_sv_eq(line, Bs_Sv_Static("```"))) {
                    break;
                }

                const Bs_Sv shell = Bs_Sv_Static("$ ");
                if (bs_sv_prefix(line, shell)) {
                    bs_sv_drop(&line, shell.size);
                    bsdoc_print_styled_sv(f, BSDOC_STYLE_STRING, shell);

                    Bs_Sv comment = line;
                    line = bs_sv_split(&comment, '#');
                    bsdoc_print_styled_sv(f, BSDOC_STYLE_NONE, line);

                    if (comment.size) {
                        comment.data--;
                        comment.size++;
                        bsdoc_print_styled_sv(f, BSDOC_STYLE_COMMENT, comment);
                    }
                } else if (bs_sv_prefix(line, Bs_Sv_Static("##"))) {
                    bs_sv_drop(&line, 1);
                    bsdoc_print_styled_sv(f, BSDOC_STYLE_COMMENT, line);
                } else if (bs_sv_prefix(line, Bs_Sv_Static("#"))) {
                    bsdoc_print_styled_sv(f, BSDOC_STYLE_COMMENT, line);
                } else {
                    bsdoc_print_styled_sv(f, BSDOC_STYLE_NONE, line);
                }

                fputc('\n', f);
            }

            fprintf(
                f,
                "</pre>\n"
                "</div>\n");

            continue;
        }

        if (bs_sv_eq(line, Bs_Sv_Static("<!-- no-navigation -->"))) {
            navigation = false;
            continue;
        }

        if (bs_sv_eq(line, Bs_Sv_Static("<!-- github-icon -->"))) {
            fprintf(
                f,
                "<a href='https://github.com/shoumodip/bs' class='navigation-icon'>\n"
                "<svg xmlns='http://www.w3.org/2000/svg' "
                "xmlns:xlink='http://www.w3.org/1999/xlink' width='30px' height='30px' viewBox='0 "
                "0 30 30' version='1.1'>"
                "<g id='surface1'>"
                "<path fill='#D4BE98' d='M 15 0 C 6.714844 0 0 6.714844 0 15 C 0 21.636719 "
                "4.304688 27.25 10.257812 29.226562 C 11.007812 29.363281 11.285156 28.902344 "
                "11.285156 28.503906 L 11.285156 25.960938 C 7.109375 26.867188 6.230469 24.152344 "
                "6.230469 24.152344 C 5.546875 22.417969 4.5625 21.957031 4.5625 21.957031 C "
                "3.191406 21.023438 4.667969 21.042969 4.667969 21.042969 C 6.179688 21.148438 "
                "6.972656 22.59375 6.972656 22.59375 C 8.316406 24.898438 10.496094 24.230469 "
                "11.355469 23.84375 C 11.492188 22.871094 11.878906 22.203125 12.3125 21.828125 C "
                "8.976562 21.449219 5.46875 20.15625 5.46875 14.398438 C 5.46875 12.757812 "
                "6.054688 11.417969 7.019531 10.367188 C 6.871094 9.988281 6.347656 8.46875 "
                "7.167969 6.410156 C 7.167969 6.410156 8.429688 6.007812 11.3125 7.960938 C "
                "12.511719 7.625 13.8125 7.460938 15.101562 7.460938 C 16.394531 7.460938 "
                "17.691406 7.625 18.894531 7.960938 C 21.777344 6.007812 23.035156 6.410156 "
                "23.035156 6.410156 C 23.859375 8.46875 23.335938 9.988281 23.1875 10.367188 C "
                "24.152344 11.417969 24.734375 12.757812 24.734375 14.398438 C 24.734375 20.167969 "
                "21.21875 21.441406 17.875 21.816406 C 18.414062 22.273438 18.894531 23.175781 "
                "18.894531 24.558594 L 18.894531 28.625 C 18.894531 29.027344 19.167969 29.488281 "
                "19.917969 29.351562 C 25.695312 27.25 30 21.636719 30 15 C 30 6.714844 23.285156 "
                "0 15 0 Z M 15 0 '/>"
                "</g>"
                "</svg>"
                "</a>\n");
            continue;
        }

        if (bs_sv_eq(line, Bs_Sv_Static("<!-- home-icon -->"))) {
            fprintf(
                f,
                "<a href='index.html' class='navigation-icon'>\n"
                "<svg xmlns='http://www.w3.org/2000/svg' "
                "xmlns:xlink='http://www.w3.org/1999/xlink' width='30px' height='30px' viewBox='0 "
                "0 30 30' version='1.1'>"
                "<g id='surface1'>"
                "<path fill='#D4BE98' d='M 29.625 13.960938 L 16.132812 2.09375 C 15.507812 "
                "1.542969 14.488281 1.542969 13.867188 2.09375 L 0.382812 13.960938 C -0.25 "
                "14.511719 -0.0820312 14.960938 0.757812 14.960938 L 4.917969 14.960938 L 4.917969 "
                "27.113281 C 4.917969 27.773438 5.464844 28.320312 6.132812 28.320312 L 11.222656 "
                "28.320312 L 11.222656 20.328125 C 11.222656 19.667969 11.769531 19.121094 "
                "12.433594 19.121094 L 17.574219 19.121094 C 18.246094 19.121094 18.789062 "
                "19.65625 18.789062 20.328125 L 18.789062 28.320312 L 23.878906 28.320312 C "
                "24.550781 28.320312 25.089844 27.777344 25.089844 27.113281 L 25.089844 14.957031 "
                "L 29.253906 14.957031 C 30.078125 14.957031 30.246094 14.511719 29.625 13.960938 "
                "Z M 29.625 13.960938 '/>"
                "</g>"
                "</a>\n");
            continue;
        }

        if (*line.data == '<') {
            fprintf(f, Bs_Sv_Fmt "\n", Bs_Sv_Arg(line));
            continue;
        }

        const bool list = bs_sv_prefix(line, Bs_Sv_Static("- "));
        if (list) {
            fprintf(f, "<ul>\n");
        } else {
            fprintf(f, "<p>\n");
        }

        bsdoc_print_line(f, line, true, list);
        while (sv.size) {
            row++;

            line = bs_sv_split(&sv, '\n');
            if (!line.size) {
                break;
            }

            if (*line.data == '<') {
                fprintf(f, Bs_Sv_Fmt "\n", Bs_Sv_Arg(line));
                continue;
            }

            bsdoc_print_line(f, line, true, list);
        }

        if (list) {
            fprintf(f, "</ul>\n");
        } else {
            fprintf(f, "</p>\n");
        }
    }

    for (size_t i = 1; i < current_level; i++) {
        fprintf(f, "</section>\n");
    }

    fprintf(f, "</main>\n");

    if (navigation) {
        fprintf(
            f,
            "<nav id='sidebar' class='collapsed'>\n"
            "<ul>\n");

        size_t i = 0;
        while (i < sections.count) {
            bsdoc_print_navigator(f, &sections, &i);
        }

        fprintf(
            f,
            "</ul>\n"
            "</nav>\n");
    } else {
        fprintf(
            f,
            "<style>\n"
            "#toggle-sidebar {\n"
            "    display: none;\n"
            "}\n"
            "@media (min-width: 768px) {\n"
            "    main {\n"
            "        margin-left: 10%%;\n"
            "        margin-right: 10%%;\n"
            "    }\n"
            "}\n"
            "</style>\n");
    }

    fprintf(
        f,
        "<script src='script.js'></script>\n"
        "</body>\n"
        "</html>\n");

    printf("Wrote documentation to '%s'\n", output);

defer:
    fclose(f);
    free(output);
    free(contents);
    return result;
}

int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        const int result = bsdoc_run_file(argv[i]);
        if (result) {
            return result;
        }
    }
}
