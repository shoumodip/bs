#include <stdio.h>

#include "compiler.h"

typedef enum {
    POWER_NIL,
    POWER_SET,
    POWER_AND,
    POWER_CMP,
    POWER_ADD,
    POWER_MUL,
    POWER_PRE,
    POWER_CALL,
} Power;

static_assert(COUNT_TOKENS == 35, "Update token_type_powers[]");
const Power token_type_powers[COUNT_TOKENS] = {
    [TOKEN_LPAREN] = POWER_CALL,

    [TOKEN_ADD] = POWER_ADD,
    [TOKEN_SUB] = POWER_ADD,

    [TOKEN_MUL] = POWER_MUL,
    [TOKEN_DIV] = POWER_MUL,

    [TOKEN_OR] = POWER_AND,
    [TOKEN_AND] = POWER_AND,

    [TOKEN_GT] = POWER_CMP,
    [TOKEN_GE] = POWER_CMP,
    [TOKEN_LT] = POWER_CMP,
    [TOKEN_LE] = POWER_CMP,
    [TOKEN_EQ] = POWER_CMP,
    [TOKEN_NE] = POWER_CMP,

    [TOKEN_SET] = POWER_SET,
};

static void scope_free(Scope *scope) {
    upvalues_free(&scope->upvalues);
    da_free(scope);
}

static bool scope_find_local(Scope *scope, SV name, size_t *index) {
    for (size_t i = scope->count; i > 0; i--) {
        if (sv_eq(scope->data[i - 1].token.sv, name)) {
            *index = i - 1;
            return true;
        }
    }

    return false;
}

static void scope_add_upvalue(Scope *scope, size_t *index, bool local) {
    for (size_t i = 0; i < scope->fn->upvalues; i++) {
        const Upvalue *upvalue = &scope->upvalues.data[i];
        if (upvalue->index == *index && upvalue->local == local) {
            *index = i;
            return;
        }
    }

    const Upvalue upvalue = {.local = local, .index = *index};
    upvalues_push(&scope->upvalues, upvalue);

    *index = scope->fn->upvalues++;
}

static bool scope_find_upvalue(Scope *scope, SV name, size_t *index) {
    if (!scope->outer) {
        return false;
    }

    if (scope_find_local(scope->outer, name, index)) {
        scope->outer->data[*index].captured = true;
        scope_add_upvalue(scope, index, true);
        return true;
    }

    if (scope_find_upvalue(scope->outer, name, index)) {
        scope_add_upvalue(scope, index, false);
        return true;
    }

    return false;
}

static void compile_error(Compiler *compiler) {
    compiler->error = true;
    compiler->lexer.quiet = true;
}

static void compile_error_expected(Compiler *compiler, const Token *token, TokenType type) {
    if (!compiler->lexer.quiet) {
        fprintf(
            stderr,
            PosFmt "error: expected %s, got %s\n",
            PosArg(token->pos),
            token_type_name(type),
            token_type_name(token->type));
    }

    compile_error(compiler);
}

static void compile_error_unexpected(Compiler *compiler, const Token *token) {
    if (!compiler->lexer.quiet) {
        fprintf(
            stderr,
            PosFmt "error: unexpected %s\n",
            PosArg(token->pos),
            token_type_name(token->type));
    }

    compile_error(compiler);
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

    compile_error_expected(compiler, &compiler->current, type);
}

static void compile_expect_inplace(Compiler *compiler, TokenType type) {
    if (compiler->current.type != type) {
        compile_error_expected(compiler, &compiler->current, type);
    }
}

static_assert(COUNT_TOKENS == 35, "Update compile_synchronize()");
static void compile_synchronize(Compiler *compiler) {
    if (compiler->lexer.quiet) {
        compiler->lexer.quiet = false;

        while (compiler->current.type != TOKEN_EOF) {
            if (compiler->previous.type == TOKEN_EOL) {
                return;
            }

            switch (compiler->current.type) {
            case TOKEN_LBRACE:

            case TOKEN_IF:
            case TOKEN_FOR:
            case TOKEN_WHILE:

            case TOKEN_FN:
            case TOKEN_VAR:
            case TOKEN_RETURN:

            case TOKEN_PRINT:
                return;

            default:
                break;
            }

            compile_advance(compiler);
        }
    }
}

static Value compile_ident_const(Compiler *compiler, SV name) {
    return value_object(gc_new_object_str(compiler->gc, name.data, name.size));
}

#define current_fn (compiler->scope->fn)
#define current_chunk (current_fn->chunk)

static void compile_scope_init(Compiler *compiler, Scope *scope) {
    scope->fn = gc_new_object_fn(compiler->gc);

    if (compiler->scope) {
        scope->fn->name =
            gc_new_object_str(compiler->gc, compiler->previous.sv.data, compiler->previous.sv.size);
    }

    scope->outer = compiler->scope;
    compiler->scope = scope;
    scope_push(scope, (Local){0});
}

static ObjectFn *compile_scope_end(Compiler *compiler) {
    chunk_push_op(&current_chunk, OP_NIL);
    chunk_push_op(&current_chunk, OP_RET);
    ObjectFn *fn = current_fn;

    Scope *outer = compiler->scope->outer;
    compiler->scope = outer;
    return fn;
}

static void compile_body_begin(Compiler *compiler) {
    compiler->scope->depth++;
}

static void compile_body_end(Compiler *compiler) {
    compiler->scope->depth--;

    while (compiler->scope->count &&
           compiler->scope->data[compiler->scope->count - 1].depth > compiler->scope->depth) {

        if (compiler->scope->data[compiler->scope->count - 1].captured) {
            chunk_push_op(&current_chunk, OP_UCLOSE);
        } else {
            chunk_push_op(&current_chunk, OP_DROP);
        }

        compiler->scope->count--;
    }
}

static size_t compile_jump_start(Compiler *compiler, Op op) {
    chunk_push_op_int(&current_chunk, op, 0);
    return current_chunk.count - 1 - sizeof(size_t);
}

static void compile_jump_patch(Compiler *compiler, size_t addr) {
    *(size_t *)&current_chunk.data[addr + 1] = current_chunk.count - 1 - sizeof(size_t) - addr;
}

static void compile_jump_direct(Compiler *compiler, Op op, size_t addr) {
    chunk_push_op_int(&current_chunk, op, addr - current_chunk.count - 1 - sizeof(size_t));
}

static_assert(COUNT_TOKENS == 35, "Update compile_expr()");
static void compile_expr(Compiler *compiler, Power mbp) {
    compile_advance(compiler);

    switch (compiler->previous.type) {
    case TOKEN_NIL:
        chunk_push_op(&current_chunk, OP_NIL);
        break;

    case TOKEN_STR:
        chunk_push_op_value(
            &current_chunk,
            OP_CONST,
            value_object(gc_new_object_str(
                compiler->gc, compiler->previous.sv.data + 1, compiler->previous.sv.size - 2)));
        break;

    case TOKEN_NUM:
        chunk_push_op_value(
            &current_chunk, OP_CONST, value_num(strtod(compiler->previous.sv.data, NULL)));
        break;

    case TOKEN_TRUE:
        chunk_push_op(&current_chunk, OP_TRUE);
        break;

    case TOKEN_FALSE:
        chunk_push_op(&current_chunk, OP_FALSE);
        break;

    case TOKEN_IDENT: {
        size_t index;
        if (scope_find_local(compiler->scope, compiler->previous.sv, &index)) {
            chunk_push_op_int(&current_chunk, OP_LGET, index);
        } else if (scope_find_upvalue(compiler->scope, compiler->previous.sv, &index)) {
            const Upvalue upvalue = compiler->scope->upvalues.data[index];
            chunk_push_op_int(&current_chunk, OP_UGET, index);
        } else {
            chunk_push_op_value(
                &current_chunk, OP_GGET, compile_ident_const(compiler, compiler->previous.sv));
        }
    } break;

    case TOKEN_LPAREN:
        compile_expr(compiler, POWER_SET);
        compile_expect(compiler, TOKEN_RPAREN);
        break;

    case TOKEN_SUB:
        compile_expr(compiler, POWER_PRE);
        chunk_push_op(&current_chunk, OP_NEG);
        break;

    case TOKEN_NOT:
        compile_expr(compiler, POWER_PRE);
        chunk_push_op(&current_chunk, OP_NOT);
        break;

    default:
        compile_error_unexpected(compiler, &compiler->previous);
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
            chunk_push_op(&current_chunk, OP_ADD);
            break;

        case TOKEN_SUB:
            compile_expr(compiler, lbp);
            chunk_push_op(&current_chunk, OP_SUB);
            break;

        case TOKEN_MUL:
            compile_expr(compiler, lbp);
            chunk_push_op(&current_chunk, OP_MUL);
            break;

        case TOKEN_DIV:
            compile_expr(compiler, lbp);
            chunk_push_op(&current_chunk, OP_DIV);
            break;

        case TOKEN_OR: {
            const size_t addr = compile_jump_start(compiler, OP_THEN);

            chunk_push_op(&current_chunk, OP_DROP);
            compile_expr(compiler, lbp);

            compile_jump_patch(compiler, addr);
        } break;

        case TOKEN_AND: {
            const size_t addr = compile_jump_start(compiler, OP_ELSE);

            chunk_push_op(&current_chunk, OP_DROP);
            compile_expr(compiler, lbp);

            compile_jump_patch(compiler, addr);
        } break;

        case TOKEN_GT:
            compile_expr(compiler, lbp);
            chunk_push_op(&current_chunk, OP_GT);
            break;

        case TOKEN_GE:
            compile_expr(compiler, lbp);
            chunk_push_op(&current_chunk, OP_GE);
            break;

        case TOKEN_LT:
            compile_expr(compiler, lbp);
            chunk_push_op(&current_chunk, OP_LT);
            break;

        case TOKEN_LE:
            compile_expr(compiler, lbp);
            chunk_push_op(&current_chunk, OP_LE);
            break;

        case TOKEN_EQ:
            compile_expr(compiler, lbp);
            chunk_push_op(&current_chunk, OP_EQ);
            break;

        case TOKEN_NE:
            compile_expr(compiler, lbp);
            chunk_push_op(&current_chunk, OP_NE);
            break;

        case TOKEN_LPAREN: {
            if (current_chunk.count <= current_chunk.last) {
                compile_error_unexpected(compiler, &compiler->previous);
                return;
            }

            const Op op = op_get_to_set(current_chunk.data[current_chunk.last]);
            if (op == OP_RET && current_chunk.data[current_chunk.last] != OP_CALL) {
                compile_error_unexpected(compiler, &compiler->previous);
                return;
            }

            size_t arity = 0;
            while (compiler->current.type != TOKEN_RPAREN && compiler->current.type != TOKEN_EOF) {
                compile_expr(compiler, POWER_SET);
                arity++;

                if (compiler->current.type != TOKEN_COMMA) {
                    break;
                }
                compile_advance(compiler);
            }
            compile_expect(compiler, TOKEN_RPAREN);

            chunk_push_op_int(&current_chunk, OP_CALL, arity);
        } break;

        case TOKEN_SET: {
            if (current_chunk.count <= current_chunk.last) {
                compile_error_unexpected(compiler, &compiler->previous);
                return;
            }

            const Op op = op_get_to_set(current_chunk.data[current_chunk.last]);
            if (op == OP_RET) {
                compile_error_unexpected(compiler, &compiler->previous);
                return;
            }

            const size_t index = *(size_t *)&current_chunk.data[current_chunk.last + 1];
            current_chunk.count = current_chunk.last;

            compile_expr(compiler, lbp);
            chunk_push_op_int(&current_chunk, op, index);
        } break;

        default:
            assert(false && "Unreachable");
        }
    }
}

static_assert(COUNT_TOKENS == 35, "Update compile_stmt()");
static void compile_stmt(Compiler *compiler) {
    switch (compiler->current.type) {
    case TOKEN_LBRACE:
        compile_advance(compiler);

        compile_body_begin(compiler);
        while (compiler->current.type != TOKEN_RBRACE && compiler->current.type != TOKEN_EOF) {
            compile_stmt(compiler);
        }
        compile_body_end(compiler);

        compile_expect(compiler, TOKEN_RBRACE);
        break;

    case TOKEN_IF: {
        compile_advance(compiler);
        compile_expr(compiler, POWER_SET);

        const size_t then_addr = compile_jump_start(compiler, OP_ELSE);
        chunk_push_op(&current_chunk, OP_DROP);

        compile_expect_inplace(compiler, TOKEN_LBRACE);
        compile_stmt(compiler);

        const size_t else_addr = compile_jump_start(compiler, OP_JUMP);
        compile_jump_patch(compiler, then_addr);
        chunk_push_op(&current_chunk, OP_DROP);

        if (compile_match(compiler, TOKEN_ELSE)) {
            compile_expect_inplace(compiler, TOKEN_LBRACE);
            compile_stmt(compiler);
        }
        compile_jump_patch(compiler, else_addr);
    } break;

    case TOKEN_FOR: {
        compile_advance(compiler);

        compile_body_begin(compiler);
        if (compiler->current.type == TOKEN_VAR) {
            compile_stmt(compiler);
        } else {
            compile_expr(compiler, POWER_NIL);
            compile_expect(compiler, TOKEN_EOL);
        }

        const size_t cond_addr = current_chunk.count;
        compile_expr(compiler, POWER_SET);
        compile_expect(compiler, TOKEN_EOL);

        const size_t loop_addr = compile_jump_start(compiler, OP_ELSE);
        chunk_push_op(&current_chunk, OP_DROP);

        const size_t iter_addr = compile_jump_start(compiler, OP_JUMP);
        compile_expr(compiler, POWER_NIL);
        chunk_push_op(&current_chunk, OP_DROP);
        compile_jump_direct(compiler, OP_JUMP, cond_addr);
        compile_jump_patch(compiler, iter_addr);

        compile_expect_inplace(compiler, TOKEN_LBRACE);
        compile_stmt(compiler);
        compile_jump_direct(compiler, OP_JUMP, iter_addr + 1 + sizeof(size_t));

        compile_jump_direct(compiler, OP_JUMP, cond_addr);
        compile_jump_patch(compiler, loop_addr);
        chunk_push_op(&current_chunk, OP_DROP);

        compile_body_end(compiler);
    } break;

    case TOKEN_WHILE: {
        compile_advance(compiler);

        const size_t cond_addr = current_chunk.count;
        compile_expr(compiler, POWER_SET);

        const size_t loop_addr = compile_jump_start(compiler, OP_ELSE);
        chunk_push_op(&current_chunk, OP_DROP);

        compile_expect_inplace(compiler, TOKEN_LBRACE);
        compile_stmt(compiler);

        compile_jump_direct(compiler, OP_JUMP, cond_addr);
        compile_jump_patch(compiler, loop_addr);
        chunk_push_op(&current_chunk, OP_DROP);
    } break;

    case TOKEN_FN: {
        compile_advance(compiler);
        compile_expect(compiler, TOKEN_IDENT);

        const Token name = compiler->previous;
        if (compiler->scope->depth) {
            da_push(
                compiler->scope,
                ((Local){.token = compiler->previous, .depth = compiler->scope->depth}));
        }

        Scope scope = {0};
        compile_scope_init(compiler, &scope);
        compile_body_begin(compiler);

        compile_expect(compiler, TOKEN_LPAREN);
        while (compiler->current.type == TOKEN_IDENT) {
            current_fn->arity++;

            compile_expect(compiler, TOKEN_IDENT);
            da_push(
                compiler->scope,
                ((Local){.token = compiler->previous, .depth = compiler->scope->depth}));

            if (compiler->current.type != TOKEN_COMMA) {
                break;
            }
            compile_advance(compiler);
        }
        compile_expect(compiler, TOKEN_RPAREN);

        compile_expect_inplace(compiler, TOKEN_LBRACE);
        compile_stmt(compiler);

        ObjectFn *fn = compile_scope_end(compiler);
        chunk_push_op_value(&current_chunk, OP_CLOSURE, value_object(fn));

        for (size_t i = 0; i < scope.fn->upvalues; i++) {
            chunk_push_op_int(
                &current_chunk, scope.upvalues.data[i].local ? 1 : 0, scope.upvalues.data[i].index);
        }

        scope_free(&scope);
        if (!compiler->scope->depth) {
            chunk_push_op_value(&current_chunk, OP_GDEF, compile_ident_const(compiler, name.sv));
        }
    } break;

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
            chunk_push_op(&current_chunk, OP_NIL);
        }
        compile_expect(compiler, TOKEN_EOL);

        if (compiler->scope->depth) {
            compiler->scope->data[index].token = name;
        } else {
            chunk_push_op_value(&current_chunk, OP_GDEF, compile_ident_const(compiler, name.sv));
        }
    } break;

    case TOKEN_RETURN:
        compile_advance(compiler);

        if (compiler->current.type == TOKEN_EOL) {
            chunk_push_op(&current_chunk, OP_NIL);
        } else {
            compile_expr(compiler, POWER_SET);
        }

        compile_expect(compiler, TOKEN_EOL);
        chunk_push_op(&current_chunk, OP_RET);
        break;

    case TOKEN_PRINT:
        compile_advance(compiler);
        compile_expr(compiler, POWER_SET);
        compile_expect(compiler, TOKEN_EOL);
        chunk_push_op(&current_chunk, OP_PRINT);
        break;

    default:
        compile_expr(compiler, POWER_NIL);
        compile_expect(compiler, TOKEN_EOL);
        chunk_push_op(&current_chunk, OP_DROP);
    }

    compile_synchronize(compiler);
}

ObjectFn *compile(Compiler *compiler) {
    Scope scope = {0};
    compile_scope_init(compiler, &scope);

    compile_advance(compiler);
    while (compiler->current.type != TOKEN_EOF) {
        compile_stmt(compiler);
    }

    ObjectFn *fn = compile_scope_end(compiler);
    scope_free(&scope);
    return compiler->error ? NULL : fn;
}
