#ifndef LEXER_H
#define LEXER_H

#include "token.h"

typedef struct {
    SV sv;
    Pos pos;
    bool quiet;
} Lexer;

Lexer lexer_new(const char *path, SV sv);
bool lexer_next(Lexer *lexer, Token *token);

#endif // LEXER_H
