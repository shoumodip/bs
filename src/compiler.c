#include <stdio.h>

#include "compiler.h"

typedef enum {
    POWER_NIL,
    POWER_SET,
    POWER_ADD,
    POWER_MUL,
    POWER_PRE
} Power;

static_assert(COUNT_TOKENS == 16, "Update token_type_powers[]");
const Power token_type_powers[COUNT_TOKENS] = {
    [TOKEN_ADD] = POWER_ADD,
    [TOKEN_SUB] = POWER_ADD,

    [TOKEN_MUL] = POWER_MUL,
    [TOKEN_DIV] = POWER_MUL,

    [TOKEN_SET] = POWER_SET,
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

static bool compile_match(Compiler *compiler, TokenType type) {
    if (compiler->current.type == type) {
        compile_advance(compiler);
        return true;
    }

    return false;
}

static void compile_expect(Compiler *compiler, TokenType type) {
    if (compile_match(compiler, type)) {
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

static_assert(COUNT_TOKENS == 16, "Update compile_synchronize()");
static void compile_synchronize(Compiler *compiler) {
    if (compiler->lexer.quiet) {
        compiler->lexer.quiet = false;

        while (compiler->current.type != TOKEN_EOF) {
            if (compiler->previous.type == TOKEN_EOL) {
                return;
            }

            switch (compiler->current.type) {
            case TOKEN_VAR:
            case TOKEN_PRINT:
                return;

            default:
                break;
            }

            compile_advance(compiler);
        }
    }
}

static void compile_unexpected(Compiler *compiler) {
    if (!compiler->lexer.quiet) {
        fprintf(
            stderr,
            PosFmt "error: unexpected %s\n",
            PosArg(compiler->previous.pos),
            token_type_name(compiler->previous.type));
    }

    compile_error(compiler);
}

static Value compile_ident_const(Compiler *compiler, Token token) {
    return value_object(gc_new_object_str(compiler->gc, token.sv.data, token.sv.size));
}

static void compile_chunk_push(Compiler *compiler, Op op) {
    compiler->last_op = compiler->chunk->count;
    da_push(compiler->chunk, op);
}

static void compile_chunk_push_int(Compiler *compiler, Op op, size_t value) {
    const size_t bytes = sizeof(value);
    compile_chunk_push(compiler, op);
    da_push_many(compiler->chunk, &value, bytes);
}

static void compile_chunk_push_value(Compiler *compiler, Op op, Value value) {
    const size_t index = compiler->chunk->constants.count;
    const size_t bytes = sizeof(index);
    values_push(&compiler->chunk->constants, value);

    compile_chunk_push(compiler, op);
    da_push_many(compiler->chunk, &index, bytes);
}

static_assert(COUNT_OPS == 16, "Update compile_expr()");
static_assert(COUNT_TOKENS == 16, "Update compile_expr()");
static void compile_expr(Compiler *compiler, Power mbp) {
    compile_advance(compiler);

    switch (compiler->previous.type) {
    case TOKEN_NIL:
        compile_chunk_push(compiler, OP_NIL);
        break;

    case TOKEN_STR:
        compile_chunk_push_value(
            compiler,
            OP_CONST,
            value_object(gc_new_object_str(
                compiler->gc, compiler->previous.sv.data + 1, compiler->previous.sv.size - 2)));
        break;

    case TOKEN_NUM:
        compile_chunk_push_value(
            compiler, OP_CONST, value_num(strtod(compiler->previous.sv.data, NULL)));
        break;

    case TOKEN_TRUE:
        compile_chunk_push(compiler, OP_TRUE);
        break;

    case TOKEN_FALSE:
        compile_chunk_push(compiler, OP_FALSE);
        break;

    case TOKEN_IDENT:
        compile_chunk_push_value(
            compiler, OP_GGET, compile_ident_const(compiler, compiler->previous));
        break;

    case TOKEN_SUB:
        compile_expr(compiler, POWER_PRE);
        compile_chunk_push(compiler, OP_NEG);
        break;

    case TOKEN_NOT:
        compile_expr(compiler, POWER_PRE);
        compile_chunk_push(compiler, OP_NOT);
        break;

    default:
        compile_unexpected(compiler);
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
            compile_chunk_push(compiler, OP_ADD);
            break;

        case TOKEN_SUB:
            compile_expr(compiler, lbp);
            compile_chunk_push(compiler, OP_SUB);
            break;

        case TOKEN_MUL:
            compile_expr(compiler, lbp);
            compile_chunk_push(compiler, OP_MUL);
            break;

        case TOKEN_DIV:
            compile_expr(compiler, lbp);
            compile_chunk_push(compiler, OP_DIV);
            break;

        case TOKEN_SET:
            if (compiler->chunk->count <= compiler->last_op) {
                return;
            }

            switch (compiler->chunk->data[compiler->last_op]) {
            case OP_GGET: {
                const size_t index = *(size_t *)&compiler->chunk->data[compiler->last_op + 1];
                compiler->chunk->count = compiler->last_op;

                compile_expr(compiler, lbp);
                compile_chunk_push_int(compiler, OP_GSET, index);
                compile_chunk_push(compiler, OP_NIL);
            } break;

            default:
                compile_unexpected(compiler);
                return;
            }
            break;

        default:
            assert(false && "Unreachable");
        }
    }
}

static_assert(COUNT_OPS == 16, "Update compile_expr()");
static_assert(COUNT_TOKENS == 16, "Update compile_stmt()");
static void compile_stmt(Compiler *compiler) {
    switch (compiler->current.type) {
    case TOKEN_VAR: {
        compile_advance(compiler);
        compile_expect(compiler, TOKEN_IDENT);

        const Value identifier = compile_ident_const(compiler, compiler->previous);
        if (compile_match(compiler, TOKEN_SET)) {
            compile_expr(compiler, POWER_SET);
        } else {
            compile_chunk_push(compiler, OP_NIL);
        }
        compile_expect(compiler, TOKEN_EOL);

        compile_chunk_push_value(compiler, OP_GDEF, identifier);
    } break;

    case TOKEN_PRINT:
        compile_advance(compiler);
        compile_expr(compiler, POWER_SET);
        compile_expect(compiler, TOKEN_EOL);
        compile_chunk_push(compiler, OP_PRINT);
        break;

    default:
        compile_expr(compiler, POWER_NIL);
        compile_expect(compiler, TOKEN_EOL);
        compile_chunk_push(compiler, OP_DROP);
    }

    compile_synchronize(compiler);
}

bool compile(Compiler *compiler, Chunk *chunk) {
    compiler->chunk = chunk;

    compile_advance(compiler);
    while (compiler->current.type != TOKEN_EOF) {
        compile_stmt(compiler);
    }

    compile_chunk_push(compiler, OP_HALT);
    return !compiler->error;
}
