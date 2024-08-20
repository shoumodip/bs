#include <stdio.h>

#include "compiler.h"

typedef enum {
    POWER_NIL,
    POWER_SET,
    POWER_ADD,
    POWER_MUL,
    POWER_PRE
} Power;

static_assert(COUNT_TOKENS == 14, "Update token_type_powers[]");
const Power token_type_powers[COUNT_TOKENS] = {
    [TOKEN_ADD] = POWER_ADD,
    [TOKEN_SUB] = POWER_ADD,

    [TOKEN_MUL] = POWER_MUL,
    [TOKEN_DIV] = POWER_MUL,
};

static void compile_error(Compiler *compiler) {
    compiler->error = true;
    compiler->lexer.quiet = true;
}

static void compile_advance(Compiler *compiler) {
    compiler->previous = compiler->current;

    while (true) {
        if (lexer_next(&compiler->lexer, &compiler->current)) {
            break;
        }

        compile_error(compiler);
    }
}

static void compile_expect(Compiler *compiler, TokenType type) {
    if (compiler->current.type == type) {
        compile_advance(compiler);
        return;
    }

    if (!compiler->lexer.quiet) {
        fprintf(
            stderr,
            PosFmt "error: expected %s, got %s\n",
            PosArg(compiler->current.pos),
            token_type_name(type),
            token_type_name(compiler->current.type));
    }

    compile_error(compiler);
}

static_assert(COUNT_TOKENS == 14, "Update compile_synchronize()");
static void compile_synchronize(Compiler *compiler) {
    if (compiler->lexer.quiet) {
        compiler->lexer.quiet = false;

        while (compiler->current.type != TOKEN_EOF) {
            if (compiler->previous.type == TOKEN_EOL) {
                return;
            }

            switch (compiler->current.type) {
            case TOKEN_EOF:
            case TOKEN_PRINT:
                return;

            default:
                break;
            }

            compile_advance(compiler);
        }
    }
}

static_assert(COUNT_OPS == 13, "Update compile_expr()");
static_assert(COUNT_TOKENS == 14, "Update compile_expr()");
static void compile_expr(Compiler *compiler, Power mbp) {
    compile_advance(compiler);

    switch (compiler->previous.type) {
    case TOKEN_NIL:
        chunk_push(compiler->chunk, OP_NIL);
        break;

    case TOKEN_STR:
        chunk_const(
            compiler->chunk,
            OP_CONST,
            value_object(gc_new_object_str(
                compiler->gc, compiler->previous.sv.data + 1, compiler->previous.sv.size - 2)));
        break;

    case TOKEN_NUM:
        chunk_const(compiler->chunk, OP_CONST, value_num(strtod(compiler->previous.sv.data, NULL)));
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
        if (!compiler->lexer.quiet) {
            fprintf(
                stderr,
                PosFmt "error: unexpected %s\n",
                PosArg(compiler->previous.pos),
                token_type_name(compiler->previous.type));
        }

        compile_error(compiler);
        return;
    }

    while (true) {
        const Power lbp = token_type_powers[compiler->current.type];
        if (lbp <= mbp) {
            break;
        }
        compile_advance(compiler);

        switch (compiler->previous.type) {
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

static_assert(COUNT_OPS == 13, "Update compile_stmt()");
static_assert(COUNT_TOKENS == 14, "Update compile_stmt()");
static void compile_stmt(Compiler *compiler) {
    switch (compiler->current.type) {
    case TOKEN_PRINT:
        compile_advance(compiler);
        compile_expr(compiler, POWER_SET);
        compile_expect(compiler, TOKEN_EOL);
        chunk_push(compiler->chunk, OP_PRINT);
        break;

    default:
        compile_expr(compiler, POWER_NIL);
        compile_expect(compiler, TOKEN_EOL);
        chunk_push(compiler->chunk, OP_DROP);
    }

    compile_synchronize(compiler);
}

bool compile(Compiler *compiler, Chunk *chunk) {
    compiler->chunk = chunk;

    compile_advance(compiler);
    while (compiler->current.type != TOKEN_EOF) {
        compile_stmt(compiler);
    }

    chunk_push(compiler->chunk, OP_HALT);
    return !compiler->error;
}
