#include <stdio.h>

#include "compiler.h"

typedef enum {
    POWER_NIL,
    POWER_CMP,
    POWER_ADD,
    POWER_MUL,
    POWER_PRE
} Power;

static_assert(COUNT_TOKENS == 13, "Update token_type_powers[]");
const Power token_type_powers[COUNT_TOKENS] = {
    [TOKEN_ADD] = POWER_ADD,
    [TOKEN_SUB] = POWER_ADD,

    [TOKEN_MUL] = POWER_MUL,
    [TOKEN_DIV] = POWER_MUL,
};

static void compile_error(Compiler *compiler) {
    if (compiler->lexer.quiet) {
        return;
    }

    compiler->error = true;
    compiler->lexer.quiet = true;
}

static_assert(COUNT_OPS == 11, "Update compile_expr()");
static_assert(COUNT_TOKENS == 13, "Update compile_expr()");
static void compile_expr(Compiler *compiler, Power mbp) {
    Token token;
    if (!lexer_next(&compiler->lexer, &token)) {
        compile_error(compiler);
        return;
    }

    switch (token.type) {
    case TOKEN_NIL:
        chunk_push(compiler->chunk, OP_NIL);
        break;

    case TOKEN_STR:
        chunk_const(
            compiler->chunk,
            OP_CONST,
            value_object(object_str_new(compiler->gc, token.sv.data + 1, token.sv.size - 2)));
        break;

    case TOKEN_NUM:
        chunk_const(compiler->chunk, OP_CONST, value_num(strtod(token.sv.data, NULL)));
        break;

    case TOKEN_TRUE:
        chunk_push(compiler->chunk, OP_TRUE);
        break;

    case TOKEN_FALSE:
        chunk_push(compiler->chunk, OP_FALSE);
        break;

    case TOKEN_SUB:
        compile_expr(compiler, POWER_PRE);
        chunk_push(compiler->chunk, OP_NEG);
        break;

    case TOKEN_NOT:
        compile_expr(compiler, POWER_PRE);
        chunk_push(compiler->chunk, OP_NOT);
        break;

    default:
        fprintf(
            stderr,
            PosFmt "error: unexpected %s\n",
            PosArg(token.pos),
            token_type_name(token.type));
        compile_error(compiler);
        return;
    }

    while (true) {
        if (!lexer_peek(&compiler->lexer, &token)) {
            compile_error(compiler);
            return;
        }

        const Power lbp = token_type_powers[token.type];
        if (lbp <= mbp) {
            break;
        }
        compiler->lexer.peeked = false;

        switch (token.type) {
        case TOKEN_ADD:
            compile_expr(compiler, lbp);
            chunk_push(compiler->chunk, OP_ADD);
            break;

        case TOKEN_SUB:
            compile_expr(compiler, lbp);
            chunk_push(compiler->chunk, OP_SUB);
            break;

        case TOKEN_MUL:
            compile_expr(compiler, lbp);
            chunk_push(compiler->chunk, OP_MUL);
            break;

        case TOKEN_DIV:
            compile_expr(compiler, lbp);
            chunk_push(compiler->chunk, OP_DIV);
            break;

        default:
            assert(false && "Unreachable");
        }
    }
}

bool compile(Compiler *compiler, Chunk *chunk) {
    compiler->chunk = chunk;

    compile_expr(compiler, POWER_NIL);
    if (!lexer_expect(&compiler->lexer, TOKEN_EOL)) {
        compile_error(compiler);
    }

    chunk_push(compiler->chunk, OP_HALT);
    return !compiler->error;
}
