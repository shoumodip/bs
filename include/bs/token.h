#ifndef BS_TOKEN_H
#define BS_TOKEN_H

#include "basic.h"

typedef struct {
    Bs_Sv path;
    Bs_Sv line;
    size_t row;
    size_t col;
} Bs_Loc;

#define Bs_Loc_Fmt "%.*s:%zu:%zu: "
#define Bs_Loc_Arg(p) (int)(p).path.size, (p).path.data, (p).row, (p).col

typedef enum {
    BS_TOKEN_EOF,
    BS_TOKEN_EOL,
    BS_TOKEN_DOT,
    BS_TOKEN_ARROW,
    BS_TOKEN_COMMA,
    BS_TOKEN_SPREAD,

    BS_TOKEN_NIL,
    BS_TOKEN_NUM,
    BS_TOKEN_STR,
    BS_TOKEN_ISTR,
    BS_TOKEN_RSTR,
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
    BS_TOKEN_MOD,

    BS_TOKEN_BOR,
    BS_TOKEN_BAND,
    BS_TOKEN_BXOR,
    BS_TOKEN_BNOT,

    BS_TOKEN_LOR,
    BS_TOKEN_LAND,
    BS_TOKEN_LNOT,

    BS_TOKEN_SHL,
    BS_TOKEN_SHR,

    BS_TOKEN_GT,
    BS_TOKEN_GE,
    BS_TOKEN_LT,
    BS_TOKEN_LE,
    BS_TOKEN_EQ,
    BS_TOKEN_NE,

    BS_TOKEN_LEN,
    BS_TOKEN_JOIN,
    BS_TOKEN_PANIC,
    BS_TOKEN_ASSERT,
    BS_TOKEN_DELETE,
    BS_TOKEN_IMPORT,
    BS_TOKEN_TYPEOF,
    BS_TOKEN_CLASSOF,

    BS_TOKEN_SET,

    // Shorthands
    BS_TOKEN_ADD_SET,
    BS_TOKEN_SUB_SET,
    BS_TOKEN_MUL_SET,
    BS_TOKEN_DIV_SET,
    BS_TOKEN_MOD_SET,
    BS_TOKEN_BOR_SET,
    BS_TOKEN_BAND_SET,
    BS_TOKEN_BXOR_SET,
    BS_TOKEN_SHL_SET,
    BS_TOKEN_SHR_SET,
    BS_TOKEN_JOIN_SET,

    BS_TOKEN_IF,
    BS_TOKEN_THEN,
    BS_TOKEN_ELSE,
    BS_TOKEN_MATCH,
    BS_TOKEN_IN,
    BS_TOKEN_IS,
    BS_TOKEN_FOR,
    BS_TOKEN_WHILE,
    BS_TOKEN_BREAK,
    BS_TOKEN_CONTINUE,

    BS_TOKEN_FN,
    BS_TOKEN_PUB,
    BS_TOKEN_VAR,
    BS_TOKEN_CONST,
    BS_TOKEN_RETURN,
    BS_TOKEN_DEFER,

    BS_TOKEN_THIS,
    BS_TOKEN_SUPER,
    BS_TOKEN_CLASS,

    BS_TOKEN_IS_MAIN_MODULE,
    BS_COUNT_TOKENS
} Bs_Token_Type;

const char *bs_token_type_name(Bs_Token_Type type);

typedef struct {
    Bs_Token_Type type;
    Bs_Sv sv;
    Bs_Loc loc;
} Bs_Token;

#endif // BS_TOKEN_H
