#ifndef BS_LEXER_H
#define BS_LEXER_H

#include <setjmp.h>

#include "token.h"

typedef enum {
    BS_ERROR_MAIN,
    BS_ERROR_PANIC,
    BS_ERROR_TRACE,
    BS_ERROR_USAGE,
} Bs_Error_Type;

typedef struct {
    Bs_Error_Type type;
    bool native;
    bool continued;

    Bs_Loc loc;
    Bs_Sv message;
    Bs_Sv explanation;
    Bs_Sv example;
} Bs_Error;

typedef struct Bs_Error_Writer Bs_Error_Writer;

struct Bs_Error_Writer {
    void *data;
    void (*write)(Bs_Error_Writer *writer, Bs_Error error);
};

typedef struct {
    Bs_Sv sv;
    Bs_Loc loc;

    bool peeked;
    Bs_Token buffer;

    bool comments;
    jmp_buf unwind;

    Bs_Error_Writer *error;
} Bs_Lexer;

Bs_Lexer bs_lexer_new(Bs_Sv path, Bs_Sv input, Bs_Error_Writer *error);
void bs_lexer_advance(Bs_Lexer *l);
void bs_lexer_buffer(Bs_Lexer *lexer, Bs_Token token);

Bs_Token bs_lexer_str(Bs_Lexer *l, Bs_Loc loc);
Bs_Token bs_lexer_next(Bs_Lexer *lexer);
Bs_Token bs_lexer_peek(Bs_Lexer *lexer);

bool bs_lexer_read(Bs_Lexer *lexer, Bs_Token_Type type);
Bs_Token bs_lexer_expect(Bs_Lexer *lexer, Bs_Token_Type type);
Bs_Token bs_lexer_either(Bs_Lexer *lexer, Bs_Token_Type a, Bs_Token_Type b);

void bs_lexer_error_full(
    Bs_Lexer *lexer, Bs_Loc loc, Bs_Sv explanation, Bs_Sv example, const char *fmt, ...)
    __attribute__((__format__(__printf__, 5, 6)));

#define bs_lexer_error(lexer, loc, ...)                                                            \
    bs_lexer_error_full(lexer, loc, (Bs_Sv){0}, (Bs_Sv){0}, __VA_ARGS__)

#endif // BS_LEXER_H
