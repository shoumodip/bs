#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>

#include "basic.h"

typedef struct {
    const char *path;
    size_t row;
    size_t col;
} Pos;

#define PosFmt "%s:%zu:%zu: "
#define PosArg(p) (p).path, (p).row, (p).col

typedef enum {
    TOKEN_EOF,
    TOKEN_EOL,

    TOKEN_NIL,
    TOKEN_NUM,
    TOKEN_STR,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_IDENT,

    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,

    TOKEN_ADD,
    TOKEN_SUB,
    TOKEN_MUL,
    TOKEN_DIV,

    TOKEN_SET,

    TOKEN_NOT,

    TOKEN_VAR,

    TOKEN_PRINT,
    COUNT_TOKENS
} TokenType;

const char *token_type_name(TokenType type);

typedef struct {
    TokenType type;
    SV sv;
    Pos pos;
} Token;

#endif // TOKEN_H
