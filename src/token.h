#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>

#include "basic.h"

typedef struct {
    const char *path;
    size_t row;
    size_t col;
} Loc;

#define LocFmt "%s:%zu:%zu: "
#define LocArg(p) (p).path, (p).row, (p).col

typedef enum {
    TOKEN_EOF,
    TOKEN_EOL,
    TOKEN_DOT,
    TOKEN_COMMA,

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
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,

    TOKEN_ADD,
    TOKEN_SUB,
    TOKEN_MUL,
    TOKEN_DIV,

    TOKEN_OR,
    TOKEN_AND,
    TOKEN_NOT,

    TOKEN_GT,
    TOKEN_GE,
    TOKEN_LT,
    TOKEN_LE,
    TOKEN_EQ,
    TOKEN_NE,

    TOKEN_LEN,
    TOKEN_JOIN,
    TOKEN_IMPORT,

    TOKEN_SET,

    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_FOR,
    TOKEN_WHILE,

    TOKEN_FN,
    TOKEN_VAR,
    TOKEN_RETURN,

    TOKEN_PRINT,
    COUNT_TOKENS
} TokenType;

const char *token_type_name(TokenType type, bool extended);

typedef struct {
    TokenType type;
    SV sv;
    Loc loc;
} Token;

#endif // TOKEN_H
