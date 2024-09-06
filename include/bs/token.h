#ifndef BS_TOKEN_H
#define BS_TOKEN_H

#include "basic.h"

typedef struct {
    const char *path;
    size_t row;
    size_t col;
} Bs_Loc;

#define Bs_Loc_Fmt "%s:%zu:%zu: "
#define Bs_Loc_Arg(p) (p).path, (p).row, (p).col

typedef enum {
    BS_TOKEN_EOF,
    BS_TOKEN_EOL,
    BS_TOKEN_DOT,
    BS_TOKEN_COMMA,

    BS_TOKEN_NIL,
    BS_TOKEN_NUM,
    BS_TOKEN_STR,
    BS_TOKEN_TRUE,
    BS_TOKEN_FALSE,
    BS_TOKEN_IDENT,

    BS_TOKEN_LPAREN,
    BS_TOKEN_RPAREN,
    BS_TOKEN_LBRACE,
    BS_TOKEN_RBRACE,
    BS_TOKEN_LBRACKET,
    BS_TOKEN_RBRACKET,

    BS_TOKEN_ADD,
    BS_TOKEN_SUB,
    BS_TOKEN_MUL,
    BS_TOKEN_DIV,

    BS_TOKEN_OR,
    BS_TOKEN_AND,
    BS_TOKEN_NOT,

    BS_TOKEN_GT,
    BS_TOKEN_GE,
    BS_TOKEN_LT,
    BS_TOKEN_LE,
    BS_TOKEN_EQ,
    BS_TOKEN_NE,

    BS_TOKEN_LEN,
    BS_TOKEN_JOIN,
    BS_TOKEN_IMPORT,

    BS_TOKEN_SET,

    BS_TOKEN_IF,
    BS_TOKEN_ELSE,
    BS_TOKEN_IN,
    BS_TOKEN_FOR,
    BS_TOKEN_WHILE,
    BS_TOKEN_BREAK,

    BS_TOKEN_FN,
    BS_TOKEN_PUB,
    BS_TOKEN_VAR,
    BS_TOKEN_RETURN,

    BS_TOKEN_IS_MAIN_MODULE,
    BS_COUNT_TOKENS
} Bs_Token_Type;

const char *bs_token_type_name(Bs_Token_Type type, bool extended);

typedef struct {
    Bs_Token_Type type;
    Bs_Sv sv;
    Bs_Loc loc;
} Bs_Token;

#endif // BS_TOKEN_H
