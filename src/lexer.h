#ifndef LEXER_H
#define LEXER_H

#include "token.h"

typedef struct {
    SV sv;
    Pos pos;
    bool quiet;

    bool peeked;
    Token buffer;
} Lexer;

Lexer lexer_new(const char *path, SV sv);
void lexer_buffer(Lexer *lexer, Token token);
bool lexer_next(Lexer *lexer, Token *token);
bool lexer_peek(Lexer *lexer, Token *token);
bool lexer_expect(Lexer *lexer, TokenType type);

#endif // LEXER_H
