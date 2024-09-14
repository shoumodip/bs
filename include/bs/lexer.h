#ifndef BS_LEXER_H
#define BS_LEXER_H

#include <setjmp.h>

#include "token.h"

typedef struct {
    Bs_Sv sv;
    Bs_Loc loc;

    bool peeked;
    Bs_Token buffer;

    bool comments;
    bool extended;
    jmp_buf unwind;

    Bs_Writer *error;
} Bs_Lexer;

Bs_Lexer bs_lexer_new(const char *path, Bs_Sv input, Bs_Writer *error);
void bs_lexer_error(Bs_Lexer *lexer);
void bs_lexer_buffer(Bs_Lexer *lexer, Bs_Token token);

Bs_Token bs_lexer_str(Bs_Lexer *l, Bs_Loc loc);
Bs_Token bs_lexer_next(Bs_Lexer *lexer);
Bs_Token bs_lexer_peek(Bs_Lexer *lexer);

bool bs_lexer_read(Bs_Lexer *lexer, Bs_Token_Type type);
Bs_Token bs_lexer_expect(Bs_Lexer *lexer, Bs_Token_Type type);
Bs_Token bs_lexer_either(Bs_Lexer *lexer, Bs_Token_Type a, Bs_Token_Type b);

#endif // BS_LEXER_H
