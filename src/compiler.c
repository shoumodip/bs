#include <stdio.h>

#include "compiler.h"

typedef enum {
    POWER_NIL,
    POWER_SET,
    POWER_ADD,
    POWER_MUL,
    POWER_PRE
} Power;

static_assert(COUNT_TOKENS == 20, "Update token_type_powers[]");
const Power token_type_powers[COUNT_TOKENS] = {
    [TOKEN_ADD] = POWER_ADD,
    [TOKEN_SUB] = POWER_ADD,

    [TOKEN_MUL] = POWER_MUL,
    [TOKEN_DIV] = POWER_MUL,

    [TOKEN_SET] = POWER_SET,
};

static bool scope_find(Scope *scope, SV name, size_t *index) {
    for (size_t i = scope->count; i > 0; i--) {
        if (sv_eq(scope->data[i - 1].token.sv, name)) {
            *index = i - 1;
            return true;
        }
    }

    return false;
}

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

static_assert(COUNT_TOKENS == 20, "Update compile_synchronize()");
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

static Value compile_ident_const(Compiler *compiler, SV name) {
    return value_object(gc_new_object_str(compiler->gc, name.data, name.size));
}

static void compile_scope_init(Compiler *compiler, Scope *scope) {
    scope->count = 0;
    scope->depth = 0;
    compiler->scope = scope;
}

static_assert(COUNT_OPS == 19, "Update compile_expr()");
static_assert(COUNT_TOKENS == 20, "Update compile_expr()");
static void compile_expr(Compiler *compiler, Power mbp) {
    compile_advance(compiler);

    switch (compiler->previous.type) {
    case TOKEN_NIL:
        chunk_push_op(compiler->chunk, OP_NIL);
        break;

    case TOKEN_STR:
        chunk_push_value(
            compiler->chunk,
            OP_CONST,
            value_object(gc_new_object_str(
                compiler->gc, compiler->previous.sv.data + 1, compiler->previous.sv.size - 2)));
        break;

    case TOKEN_NUM:
        chunk_push_value(
            compiler->chunk, OP_CONST, value_num(strtod(compiler->previous.sv.data, NULL)));
        break;

    case TOKEN_TRUE:
        chunk_push_op(compiler->chunk, OP_TRUE);
        break;

    case TOKEN_FALSE:
        chunk_push_op(compiler->chunk, OP_FALSE);
        break;

    case TOKEN_IDENT: {
        size_t local_index;
        if (scope_find(compiler->scope, compiler->previous.sv, &local_index)) {
            chunk_push_int(compiler->chunk, OP_LGET, local_index);
        } else {
            chunk_push_value(
                compiler->chunk, OP_GGET, compile_ident_const(compiler, compiler->previous.sv));
        }
    } break;

    case TOKEN_LPAREN:
        compile_expr(compiler, POWER_SET);
        compile_expect(compiler, TOKEN_RPAREN);
        break;

    case TOKEN_SUB:
        compile_expr(compiler, POWER_PRE);
        chunk_push_op(compiler->chunk, OP_NEG);
        break;

    case TOKEN_NOT:
        compile_expr(compiler, POWER_PRE);
        chunk_push_op(compiler->chunk, OP_NOT);
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
            chunk_push_op(compiler->chunk, OP_ADD);
            break;

        case TOKEN_SUB:
            compile_expr(compiler, lbp);
            chunk_push_op(compiler->chunk, OP_SUB);
            break;

        case TOKEN_MUL:
            compile_expr(compiler, lbp);
            chunk_push_op(compiler->chunk, OP_MUL);
            break;

        case TOKEN_DIV:
            compile_expr(compiler, lbp);
            chunk_push_op(compiler->chunk, OP_DIV);
            break;

        case TOKEN_SET:
            if (compiler->chunk->count <= compiler->chunk->last) {
                return;
            }

            switch (compiler->chunk->data[compiler->chunk->last]) {
            case OP_GGET: {
                const size_t index = *(size_t *)&compiler->chunk->data[compiler->chunk->last + 1];
                compiler->chunk->count = compiler->chunk->last;

                compile_expr(compiler, lbp);
                chunk_push_int(compiler->chunk, OP_GSET, index);
            } break;

            case OP_LGET: {
                const size_t index = *(size_t *)&compiler->chunk->data[compiler->chunk->last + 1];
                compiler->chunk->count = compiler->chunk->last;

                compile_expr(compiler, lbp);
                chunk_push_int(compiler->chunk, OP_LSET, index);
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

static_assert(COUNT_OPS == 19, "Update compile_expr()");
static_assert(COUNT_TOKENS == 20, "Update compile_stmt()");
static void compile_stmt(Compiler *compiler) {
    switch (compiler->current.type) {
    case TOKEN_LBRACE:
        compile_advance(compiler);

        compiler->scope->depth++;
        while (compiler->current.type != TOKEN_RBRACE && compiler->current.type != TOKEN_EOF) {
            compile_stmt(compiler);
        }
        compiler->scope->depth--;

        size_t drops = 0;
        while (compiler->scope->count &&
               compiler->scope->data[compiler->scope->count - 1].depth > compiler->scope->depth) {
            drops++;
            compiler->scope->count--;
        }

        if (drops == 1) {
            chunk_push_op(compiler->chunk, OP_DROP);
        } else if (drops) {
            chunk_push_int(compiler->chunk, OP_DROPS, drops);
        }
        compile_expect(compiler, TOKEN_RBRACE);
        break;

    case TOKEN_VAR: {
        compile_advance(compiler);
        compile_expect(compiler, TOKEN_IDENT);

        const Token name = compiler->previous;
        const size_t index = compiler->scope->count;
        if (compiler->scope->depth) {
            da_push(compiler->scope, ((Local){.depth = compiler->scope->depth}));
        }

        if (compile_match(compiler, TOKEN_SET)) {
            compile_expr(compiler, POWER_SET);
        } else {
            chunk_push_op(compiler->chunk, OP_NIL);
        }
        compile_expect(compiler, TOKEN_EOL);

        if (compiler->scope->depth) {
            compiler->scope->data[index].token = name;
        } else {
            chunk_push_value(compiler->chunk, OP_GDEF, compile_ident_const(compiler, name.sv));
        }
    } break;

    case TOKEN_PRINT:
        compile_advance(compiler);
        compile_expr(compiler, POWER_SET);
        compile_expect(compiler, TOKEN_EOL);
        chunk_push_op(compiler->chunk, OP_PRINT);
        break;

    default:
        compile_expr(compiler, POWER_NIL);
        compile_expect(compiler, TOKEN_EOL);
        chunk_push_op(compiler->chunk, OP_DROP);
    }

    compile_synchronize(compiler);
}

bool compile(Compiler *compiler, Chunk *chunk) {
    Scope scope = {0};
    compile_scope_init(compiler, &scope);

    compiler->chunk = chunk;

    compile_advance(compiler);
    while (compiler->current.type != TOKEN_EOF) {
        compile_stmt(compiler);
    }
    chunk_push_op(compiler->chunk, OP_HALT);

    da_free(&scope);
    return !compiler->error;
}
