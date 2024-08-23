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

static void scope_free(Scope *s) {
    if (s) {
        upvalues_free(&s->upvalues);
        da_free(s);
    }
}

static bool scope_find_local(Scope *s, SV name, size_t *index) {
    for (size_t i = s->count; i > 0; i--) {
        if (sv_eq(s->data[i - 1].token.sv, name)) {
            *index = i - 1;
            return true;
        }
    }

    return false;
}

static void scope_add_upvalue(Scope *s, size_t *index, bool local) {
    for (size_t i = 0; i < s->fn->upvalues; i++) {
        const Upvalue *upvalue = &s->upvalues.data[i];
        if (upvalue->index == *index && upvalue->local == local) {
            *index = i;
            return;
        }
    }

    const Upvalue upvalue = {.local = local, .index = *index};
    upvalues_push(&s->upvalues, upvalue);

    *index = s->fn->upvalues++;
}

static bool scope_find_upvalue(Scope *s, SV name, size_t *index) {
    if (!s->outer) {
        return false;
    }

    if (scope_find_local(s->outer, name, index)) {
        s->outer->data[*index].captured = true;
        scope_add_upvalue(s, index, true);
        return true;
    }

    if (scope_find_upvalue(s->outer, name, index)) {
        scope_add_upvalue(s, index, false);
        return true;
    }

    return false;
}

static Value compile_ident_const(Compiler *c, SV name) {
    return value_object(gc_new_object_str(c->gc, name.data, name.size));
}

static void compile_scope_init(Compiler *c, Scope *scope, const Token *token) {
    scope->fn = gc_new_object_fn(c->gc);

    if (token) {
        scope->fn->name = gc_new_object_str(c->gc, token->sv.data, token->sv.size);
    }

    scope->outer = c->scope;
    c->scope = scope;
    c->chunk = &c->scope->fn->chunk;
    scope_push(scope, (Local){0});
}

static ObjectFn *compile_scope_end(Compiler *c) {
    chunk_push_op(c->chunk, OP_NIL);
    chunk_push_op(c->chunk, OP_RET);
    ObjectFn *fn = c->scope->fn;

    Scope *outer = c->scope->outer;
    c->scope = outer;

    if (c->scope) {
        c->chunk = &c->scope->fn->chunk;
    } else {
        c->chunk = NULL;
    }
    return fn;
}

static void compile_body_begin(Compiler *c) {
    c->scope->depth++;
}

static void compile_body_end(Compiler *c) {
    c->scope->depth--;

    while (c->scope->count && c->scope->data[c->scope->count - 1].depth > c->scope->depth) {
        if (c->scope->data[c->scope->count - 1].captured) {
            chunk_push_op(c->chunk, OP_UCLOSE);
        } else {
            chunk_push_op(c->chunk, OP_DROP);
        }

        c->scope->count--;
    }
}

static size_t compile_jump_start(Compiler *c, Op op) {
    chunk_push_op_int(c->chunk, op, 0);
    return c->chunk->count - 1 - sizeof(size_t);
}

static void compile_jump_patch(Compiler *c, size_t addr) {
    *(size_t *)&c->chunk->data[addr + 1] = c->chunk->count - 1 - sizeof(size_t) - addr;
}

static void compile_jump_direct(Compiler *c, Op op, size_t addr) {
    chunk_push_op_int(c->chunk, op, addr - c->chunk->count - 1 - sizeof(size_t));
}

static_assert(COUNT_TOKENS == 35, "Update compile_expr()");
static void compile_expr(Compiler *c, Power mbp) {
    Token token = lexer_next(&c->lexer);

    switch (token.type) {
    case TOKEN_NIL:
        chunk_push_op(c->chunk, OP_NIL);
        break;

    case TOKEN_STR:
        chunk_push_op_value(
            c->chunk,
            OP_CONST,
            value_object(gc_new_object_str(c->gc, token.sv.data + 1, token.sv.size - 2)));
        break;

    case TOKEN_NUM:
        chunk_push_op_value(c->chunk, OP_CONST, value_num(strtod(token.sv.data, NULL)));
        break;

    case TOKEN_TRUE:
        chunk_push_op(c->chunk, OP_TRUE);
        break;

    case TOKEN_FALSE:
        chunk_push_op(c->chunk, OP_FALSE);
        break;

    case TOKEN_IDENT: {
        size_t index;
        if (scope_find_local(c->scope, token.sv, &index)) {
            chunk_push_op_int(c->chunk, OP_LGET, index);
        } else if (scope_find_upvalue(c->scope, token.sv, &index)) {
            chunk_push_op_int(c->chunk, OP_UGET, index);
        } else {
            chunk_push_op_value(c->chunk, OP_GGET, compile_ident_const(c, token.sv));
        }
    } break;

    case TOKEN_LPAREN:
        compile_expr(c, POWER_SET);
        lexer_expect(&c->lexer, TOKEN_RPAREN);
        break;

    case TOKEN_SUB:
        compile_expr(c, POWER_PRE);
        chunk_push_op(c->chunk, OP_NEG);
        break;

    case TOKEN_NOT:
        compile_expr(c, POWER_PRE);
        chunk_push_op(c->chunk, OP_NOT);
        break;

    default:
        fprintf(
            stderr,
            PosFmt "error: unexpected %s\n",
            PosArg(token.pos),
            token_type_name(token.type));

        lexer_error(&c->lexer);
        return;
    }

    while (true) {
        token = lexer_peek(&c->lexer);

        const Power lbp = token_type_powers[token.type];
        if (lbp <= mbp) {
            break;
        }
        c->lexer.peeked = false;

        switch (token.type) {
        case TOKEN_ADD:
            compile_expr(c, lbp);
            chunk_push_op(c->chunk, OP_ADD);
            break;

        case TOKEN_SUB:
            compile_expr(c, lbp);
            chunk_push_op(c->chunk, OP_SUB);
            break;

        case TOKEN_MUL:
            compile_expr(c, lbp);
            chunk_push_op(c->chunk, OP_MUL);
            break;

        case TOKEN_DIV:
            compile_expr(c, lbp);
            chunk_push_op(c->chunk, OP_DIV);
            break;

        case TOKEN_OR: {
            const size_t addr = compile_jump_start(c, OP_THEN);

            chunk_push_op(c->chunk, OP_DROP);
            compile_expr(c, lbp);

            compile_jump_patch(c, addr);
        } break;

        case TOKEN_AND: {
            const size_t addr = compile_jump_start(c, OP_ELSE);

            chunk_push_op(c->chunk, OP_DROP);
            compile_expr(c, lbp);

            compile_jump_patch(c, addr);
        } break;

        case TOKEN_GT:
            compile_expr(c, lbp);
            chunk_push_op(c->chunk, OP_GT);
            break;

        case TOKEN_GE:
            compile_expr(c, lbp);
            chunk_push_op(c->chunk, OP_GE);
            break;

        case TOKEN_LT:
            compile_expr(c, lbp);
            chunk_push_op(c->chunk, OP_LT);
            break;

        case TOKEN_LE:
            compile_expr(c, lbp);
            chunk_push_op(c->chunk, OP_LE);
            break;

        case TOKEN_EQ:
            compile_expr(c, lbp);
            chunk_push_op(c->chunk, OP_EQ);
            break;

        case TOKEN_NE:
            compile_expr(c, lbp);
            chunk_push_op(c->chunk, OP_NE);
            break;

        case TOKEN_LPAREN: {
            if (c->chunk->count <= c->chunk->last) {
                return;
            }

            const Op op = op_get_to_set(c->chunk->data[c->chunk->last]);
            if (op == OP_RET && c->chunk->data[c->chunk->last] != OP_CALL) {
                return;
            }

            size_t arity = 0;
            while (!lexer_read(&c->lexer, TOKEN_RPAREN)) {
                compile_expr(c, POWER_SET);
                arity++;

                token = lexer_peek(&c->lexer);
                if (token.type != TOKEN_COMMA) {
                    lexer_expect(&c->lexer, TOKEN_RPAREN);
                    break;
                }
                c->lexer.peeked = false;
            }

            chunk_push_op_int(c->chunk, OP_CALL, arity);
        } break;

        case TOKEN_SET: {
            if (c->chunk->count <= c->chunk->last) {
                return;
            }

            const Op op = op_get_to_set(c->chunk->data[c->chunk->last]);
            if (op == OP_RET) {
                return;
            }

            const size_t index = *(size_t *)&c->chunk->data[c->chunk->last + 1];
            c->chunk->count = c->chunk->last;

            compile_expr(c, lbp);
            chunk_push_op_int(c->chunk, op, index);
        } break;

        default:
            assert(false && "Unreachable");
        }
    }
}

static_assert(COUNT_TOKENS == 35, "Update compile_stmt()");
static void compile_stmt(Compiler *c) {
    Token token = lexer_next(&c->lexer);

    switch (token.type) {
    case TOKEN_LBRACE:
        compile_body_begin(c);
        while (!lexer_read(&c->lexer, TOKEN_RBRACE)) {
            compile_stmt(c);
        }
        compile_body_end(c);
        break;

    case TOKEN_IF: {
        compile_expr(c, POWER_SET);

        const size_t then_addr = compile_jump_start(c, OP_ELSE);
        chunk_push_op(c->chunk, OP_DROP);

        lexer_buffer(&c->lexer, lexer_expect(&c->lexer, TOKEN_LBRACE));
        compile_stmt(c);

        const size_t else_addr = compile_jump_start(c, OP_JUMP);
        compile_jump_patch(c, then_addr);
        chunk_push_op(c->chunk, OP_DROP);

        if (lexer_read(&c->lexer, TOKEN_ELSE)) {
            lexer_buffer(&c->lexer, lexer_expect(&c->lexer, TOKEN_LBRACE));
            compile_stmt(c);
        }
        compile_jump_patch(c, else_addr);
    } break;

    case TOKEN_FOR: {
        compile_body_begin(c);

        token = lexer_peek(&c->lexer);
        if (token.type == TOKEN_VAR) {
            compile_stmt(c);
        } else {
            compile_expr(c, POWER_NIL);
            lexer_expect(&c->lexer, TOKEN_EOL);
        }

        const size_t cond_addr = c->chunk->count;
        compile_expr(c, POWER_SET);
        lexer_expect(&c->lexer, TOKEN_EOL);

        const size_t loop_addr = compile_jump_start(c, OP_ELSE);
        chunk_push_op(c->chunk, OP_DROP);

        const size_t iter_addr = compile_jump_start(c, OP_JUMP);
        compile_expr(c, POWER_NIL);
        chunk_push_op(c->chunk, OP_DROP);
        compile_jump_direct(c, OP_JUMP, cond_addr);
        compile_jump_patch(c, iter_addr);

        lexer_buffer(&c->lexer, lexer_expect(&c->lexer, TOKEN_LBRACE));
        compile_stmt(c);
        compile_jump_direct(c, OP_JUMP, iter_addr + 1 + sizeof(size_t));

        compile_jump_direct(c, OP_JUMP, cond_addr);
        compile_jump_patch(c, loop_addr);
        chunk_push_op(c->chunk, OP_DROP);

        compile_body_end(c);
    } break;

    case TOKEN_WHILE: {
        const size_t cond_addr = c->chunk->count;
        compile_expr(c, POWER_SET);

        const size_t loop_addr = compile_jump_start(c, OP_ELSE);
        chunk_push_op(c->chunk, OP_DROP);

        lexer_buffer(&c->lexer, lexer_expect(&c->lexer, TOKEN_LBRACE));
        compile_stmt(c);

        compile_jump_direct(c, OP_JUMP, cond_addr);
        compile_jump_patch(c, loop_addr);
        chunk_push_op(c->chunk, OP_DROP);
    } break;

    case TOKEN_FN: {
        const Token name = lexer_expect(&c->lexer, TOKEN_IDENT);
        if (c->scope->depth) {
            da_push(c->scope, ((Local){.token = name, .depth = c->scope->depth}));
        }

        Scope scope = {0};
        compile_scope_init(c, &scope, &name);
        compile_body_begin(c);

        lexer_expect(&c->lexer, TOKEN_LPAREN);
        while (!lexer_read(&c->lexer, TOKEN_RPAREN)) {
            c->scope->fn->arity++;

            const Token arg = lexer_expect(&c->lexer, TOKEN_IDENT);
            da_push(c->scope, ((Local){.token = arg, .depth = c->scope->depth}));

            token = lexer_peek(&c->lexer);
            if (token.type != TOKEN_COMMA) {
                lexer_expect(&c->lexer, TOKEN_RPAREN);
                break;
            }
            c->lexer.peeked = false;
        }

        lexer_buffer(&c->lexer, lexer_expect(&c->lexer, TOKEN_LBRACE));
        compile_stmt(c);

        ObjectFn *fn = compile_scope_end(c);
        chunk_push_op_value(c->chunk, OP_CLOSURE, value_object(fn));

        for (size_t i = 0; i < scope.fn->upvalues; i++) {
            chunk_push_op_int(
                c->chunk, scope.upvalues.data[i].local ? 1 : 0, scope.upvalues.data[i].index);
        }

        scope_free(&scope);
        if (!c->scope->depth) {
            chunk_push_op_value(c->chunk, OP_GDEF, compile_ident_const(c, name.sv));
        }
    } break;

    case TOKEN_VAR: {
        const Token name = lexer_expect(&c->lexer, TOKEN_IDENT);

        const size_t index = c->scope->count;
        if (c->scope->depth) {
            da_push(c->scope, ((Local){.depth = c->scope->depth}));
        }

        if (lexer_read(&c->lexer, TOKEN_SET)) {
            compile_expr(c, POWER_SET);
        } else {
            chunk_push_op(c->chunk, OP_NIL);
        }
        lexer_expect(&c->lexer, TOKEN_EOL);

        if (c->scope->depth) {
            c->scope->data[index].token = name;
        } else {
            chunk_push_op_value(c->chunk, OP_GDEF, compile_ident_const(c, name.sv));
        }
    } break;

    case TOKEN_RETURN:
        token = lexer_peek(&c->lexer);

        if (token.type == TOKEN_EOL) {
            chunk_push_op(c->chunk, OP_NIL);
        } else {
            compile_expr(c, POWER_SET);
        }

        lexer_expect(&c->lexer, TOKEN_EOL);
        chunk_push_op(c->chunk, OP_RET);
        break;

    case TOKEN_PRINT:
        compile_expr(c, POWER_SET);
        lexer_expect(&c->lexer, TOKEN_EOL);
        chunk_push_op(c->chunk, OP_PRINT);
        break;

    default:
        lexer_buffer(&c->lexer, token);
        compile_expr(c, POWER_NIL);
        lexer_expect(&c->lexer, TOKEN_EOL);
        chunk_push_op(c->chunk, OP_DROP);
    }
}

ObjectFn *compile(Compiler *c) {
    Scope scope = {0};
    compile_scope_init(c, &scope, NULL);

    if (setjmp(c->lexer.error)) {
        Scope *scope = c->scope;
        while (scope) {
            Scope *outer = scope->outer;
            scope_free(scope);
            scope = outer;
        }

        c->scope = NULL;
        c->chunk = NULL;
        return NULL;
    }

    while (!lexer_read(&c->lexer, TOKEN_EOF)) {
        compile_stmt(c);
    }

    ObjectFn *fn = compile_scope_end(c);
    scope_free(&scope);
    return fn;
}
