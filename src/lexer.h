#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include <setjmp.h>

typedef struct {
    SV sv;
    Pos pos;

    bool peeked;
    Token buffer;

    jmp_buf error;
} Lexer;

Lexer lexer_new(const char *path, SV sv);
void lexer_error(Lexer *lexer);
void lexer_buffer(Lexer *lexer, Token token);

Token lexer_next(Lexer *lexer);
Token lexer_peek(Lexer *lexer);

bool lexer_read(Lexer *lexer, TokenType type);
Token lexer_expect(Lexer *lexer, TokenType type);

#endif // LEXER_H
