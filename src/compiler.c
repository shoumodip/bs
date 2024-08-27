#include <assert.h>

#include "compiler.h"
#include "lexer.h"

typedef enum {
    POWER_NIL,
    POWER_SET,
    POWER_AND,
    POWER_CMP,
    POWER_ADD,
    POWER_MUL,
    POWER_PRE,
    POWER_DOT,
} Power;

static_assert(COUNT_TOKENS == 41, "Update token_type_powers[]");
const Power token_type_powers[COUNT_TOKENS] = {
    [TOKEN_DOT] = POWER_DOT,
    [TOKEN_LPAREN] = POWER_DOT,
    [TOKEN_LBRACKET] = POWER_DOT,

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

    [TOKEN_JOIN] = POWER_ADD,

    [TOKEN_SET] = POWER_SET,
};

typedef struct {
    Token token;
    size_t depth;
    bool captured;
} Local;

typedef struct {
    bool local;
    size_t index;
} Upvalue;

typedef struct {
    Upvalue *data;
    size_t count;
    size_t capacity;
} Upvalues;

#define upvalues_free da_free
#define upvalues_push da_push

typedef struct Scope Scope;

struct Scope {
    Scope *outer;

    Local *data;
    size_t count;
    size_t depth;
    size_t capacity;

    ObjectFn *fn;
    Upvalues upvalues;
};

#define scope_push da_push

static void scope_free(Vm *vm, Scope *s) {
    if (s) {
        upvalues_free(vm, &s->upvalues);
        da_free(vm, s);
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

static void scope_add_upvalue(Vm *vm, Scope *s, size_t *index, bool local) {
    for (size_t i = 0; i < s->fn->upvalues; i++) {
        const Upvalue *upvalue = &s->upvalues.data[i];
        if (upvalue->index == *index && upvalue->local == local) {
            *index = i;
            return;
        }
    }

    const Upvalue upvalue = {.local = local, .index = *index};
    upvalues_push(vm, &s->upvalues, upvalue);

    *index = s->fn->upvalues++;
}

static bool scope_find_upvalue(Vm *vm, Scope *s, SV name, size_t *index) {
    if (!s->outer) {
        return false;
    }

    if (scope_find_local(s->outer, name, index)) {
        s->outer->data[*index].captured = true;
        scope_add_upvalue(vm, s, index, true);
        return true;
    }

    if (scope_find_upvalue(vm, s->outer, name, index)) {
        scope_add_upvalue(vm, s, index, false);
        return true;
    }

    return false;
}

typedef struct {
    Lexer lexer;
    Scope *scope;

    Vm *vm;
    Chunk *chunk;
} Compiler;

static size_t compile_jump_start(Compiler *c, Op op) {
    chunk_push_op_int(c->vm, c->chunk, op, 0);
    return c->chunk->count - 1 - sizeof(size_t);
}

static void compile_jump_patch(Compiler *c, size_t addr) {
    *(size_t *)&c->chunk->data[addr + 1] = c->chunk->count - 1 - sizeof(size_t) - addr;
}

static void compile_jump_direct(Compiler *c, Op op, size_t addr) {
    chunk_push_op_int(c->vm, c->chunk, op, addr - c->chunk->count - 1 - sizeof(size_t));
}

static void compile_error_unexpected(Compiler *c, const Token *token) {
    fprintf(
        stderr,
        LocFmt "error: unexpected %s\n",
        LocArg(token->loc),
        token_type_name(token->type, c->lexer.extended));

    lexer_error(&c->lexer);
}

static void compile_function(Compiler *c, const Token *name);

static_assert(COUNT_TOKENS == 41, "Update compile_expr()");
static void compile_expr(Compiler *c, Power mbp) {
    Token token = lexer_next(&c->lexer);
    Loc loc = token.loc;

    switch (token.type) {
    case TOKEN_NIL:
        chunk_push_op(c->vm, c->chunk, OP_NIL);
        break;

    case TOKEN_STR:
        chunk_push_op_value(
            c->vm,
            c->chunk,
            OP_CONST,
            value_object(object_str_const(c->vm, token.sv.data + 1, token.sv.size - 2)));
        break;

    case TOKEN_NUM:
        chunk_push_op_value(c->vm, c->chunk, OP_CONST, value_num(strtod(token.sv.data, NULL)));
        break;

    case TOKEN_TRUE:
        chunk_push_op(c->vm, c->chunk, OP_TRUE);
        break;

    case TOKEN_FALSE:
        chunk_push_op(c->vm, c->chunk, OP_FALSE);
        break;

    case TOKEN_IDENT: {
        size_t index;
        if (scope_find_local(c->scope, token.sv, &index)) {
            chunk_push_op_int(c->vm, c->chunk, OP_LGET, index);
        } else if (scope_find_upvalue(c->vm, c->scope, token.sv, &index)) {
            chunk_push_op_int(c->vm, c->chunk, OP_UGET, index);
        } else {
            chunk_push_op_loc(c->vm, c->chunk, loc);
            chunk_push_op_value(
                c->vm,
                c->chunk,
                OP_GGET,
                value_object(object_str_const(c->vm, token.sv.data, token.sv.size)));
        }
    } break;

    case TOKEN_LPAREN:
        compile_expr(c, POWER_SET);
        lexer_expect(&c->lexer, TOKEN_RPAREN);
        break;

    case TOKEN_LBRACE: {
        chunk_push_op(c->vm, c->chunk, OP_TABLE);

        while (!lexer_read(&c->lexer, TOKEN_RBRACE)) {
            token = lexer_peek(&c->lexer);
            loc = token.loc;

            if (token.type == TOKEN_LBRACKET) {
                c->lexer.peeked = false;
                compile_expr(c, POWER_SET);
                lexer_expect(&c->lexer, TOKEN_RBRACKET);
            } else {
                token = lexer_expect(&c->lexer, TOKEN_IDENT);
                chunk_push_op_value(
                    c->vm,
                    c->chunk,
                    OP_CONST,
                    value_object(object_str_const(c->vm, token.sv.data, token.sv.size)));
            }

            lexer_expect(&c->lexer, TOKEN_SET);
            compile_expr(c, POWER_SET);

            chunk_push_op_loc(c->vm, c->chunk, loc);
            chunk_push_op(c->vm, c->chunk, OP_ILIT);

            token = lexer_peek(&c->lexer);
            if (token.type != TOKEN_COMMA) {
                lexer_expect(&c->lexer, TOKEN_RBRACE);
                break;
            }
            c->lexer.peeked = false;
        }
    } break;

    case TOKEN_LBRACKET: {
        chunk_push_op(c->vm, c->chunk, OP_ARRAY);

        size_t index = 0;
        while (!lexer_read(&c->lexer, TOKEN_RBRACKET)) {
            chunk_push_op_value(c->vm, c->chunk, OP_CONST, value_num(index));
            compile_expr(c, POWER_SET);
            index++;

            chunk_push_op(c->vm, c->chunk, OP_ILIT);

            token = lexer_peek(&c->lexer);
            if (token.type != TOKEN_COMMA) {
                lexer_expect(&c->lexer, TOKEN_RBRACKET);
                break;
            }
            c->lexer.peeked = false;
        }
    } break;

    case TOKEN_SUB:
        compile_expr(c, POWER_PRE);
        chunk_push_op_loc(c->vm, c->chunk, loc);
        chunk_push_op(c->vm, c->chunk, OP_NEG);
        break;

    case TOKEN_NOT:
        compile_expr(c, POWER_PRE);
        chunk_push_op(c->vm, c->chunk, OP_NOT);
        break;

    case TOKEN_LEN:
        lexer_expect(&c->lexer, TOKEN_LPAREN);
        compile_expr(c, POWER_SET);
        lexer_expect(&c->lexer, TOKEN_RPAREN);

        chunk_push_op_loc(c->vm, c->chunk, loc);
        chunk_push_op(c->vm, c->chunk, OP_LEN);
        break;

    case TOKEN_IMPORT:
        lexer_expect(&c->lexer, TOKEN_LPAREN);
        compile_expr(c, POWER_SET);
        lexer_expect(&c->lexer, TOKEN_RPAREN);

        chunk_push_op_loc(c->vm, c->chunk, loc);
        chunk_push_op(c->vm, c->chunk, OP_IMPORT);
        break;

    case TOKEN_FN:
        compile_function(c, NULL);
        break;

    default:
        compile_error_unexpected(c, &token);
        return;
    }

    while (true) {
        token = lexer_peek(&c->lexer);

        const Power lbp = token_type_powers[token.type];
        if (lbp <= mbp) {
            break;
        }
        c->lexer.peeked = false;

        loc = token.loc;
        switch (token.type) {
        case TOKEN_DOT: {
            const Op op_get = c->chunk->data[c->chunk->last];
            const Op op_set = op_get_to_set(op_get);
            if (op_set == OP_RET && op_get != OP_CALL && op_get != OP_IMPORT && op_get != OP_ILIT) {
                compile_error_unexpected(c, &token);
            }

            token = lexer_expect(&c->lexer, TOKEN_IDENT);
            chunk_push_op_value(
                c->vm,
                c->chunk,
                OP_CONST,
                value_object(object_str_const(c->vm, token.sv.data, token.sv.size)));

            chunk_push_op_loc(c->vm, c->chunk, loc);
            chunk_push_op(c->vm, c->chunk, OP_IGET);
        } break;

        case TOKEN_ADD:
            compile_expr(c, lbp);
            chunk_push_op_loc(c->vm, c->chunk, loc);
            chunk_push_op(c->vm, c->chunk, OP_ADD);
            break;

        case TOKEN_SUB:
            compile_expr(c, lbp);
            chunk_push_op_loc(c->vm, c->chunk, loc);
            chunk_push_op(c->vm, c->chunk, OP_SUB);
            break;

        case TOKEN_MUL:
            compile_expr(c, lbp);
            chunk_push_op_loc(c->vm, c->chunk, loc);
            chunk_push_op(c->vm, c->chunk, OP_MUL);
            break;

        case TOKEN_DIV:
            compile_expr(c, lbp);
            chunk_push_op_loc(c->vm, c->chunk, loc);
            chunk_push_op(c->vm, c->chunk, OP_DIV);
            break;

        case TOKEN_OR: {
            const size_t addr = compile_jump_start(c, OP_THEN);

            chunk_push_op(c->vm, c->chunk, OP_DROP);
            compile_expr(c, lbp);

            compile_jump_patch(c, addr);
        } break;

        case TOKEN_AND: {
            const size_t addr = compile_jump_start(c, OP_ELSE);

            chunk_push_op(c->vm, c->chunk, OP_DROP);
            compile_expr(c, lbp);

            compile_jump_patch(c, addr);
        } break;

        case TOKEN_GT:
            compile_expr(c, lbp);
            chunk_push_op_loc(c->vm, c->chunk, loc);
            chunk_push_op(c->vm, c->chunk, OP_GT);
            break;

        case TOKEN_GE:
            compile_expr(c, lbp);
            chunk_push_op_loc(c->vm, c->chunk, loc);
            chunk_push_op(c->vm, c->chunk, OP_GE);
            break;

        case TOKEN_LT:
            compile_expr(c, lbp);
            chunk_push_op_loc(c->vm, c->chunk, loc);
            chunk_push_op(c->vm, c->chunk, OP_LT);
            break;

        case TOKEN_LE:
            compile_expr(c, lbp);
            chunk_push_op_loc(c->vm, c->chunk, loc);
            chunk_push_op(c->vm, c->chunk, OP_LE);
            break;

        case TOKEN_EQ:
            compile_expr(c, lbp);
            chunk_push_op_loc(c->vm, c->chunk, loc);
            chunk_push_op(c->vm, c->chunk, OP_EQ);
            break;

        case TOKEN_NE:
            compile_expr(c, lbp);
            chunk_push_op_loc(c->vm, c->chunk, loc);
            chunk_push_op(c->vm, c->chunk, OP_NE);
            break;

        case TOKEN_LPAREN: {
            const Op op_get = c->chunk->data[c->chunk->last];
            const Op op_set = op_get_to_set(op_get);
            if (op_set == OP_RET && op_get != OP_CALL && op_get != OP_CLOSURE) {
                compile_error_unexpected(c, &token);
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

            chunk_push_op_loc(c->vm, c->chunk, loc);
            chunk_push_op_int(c->vm, c->chunk, OP_CALL, arity);
        } break;

        case TOKEN_LBRACKET: {
            const Op op_get = c->chunk->data[c->chunk->last];
            const Op op_set = op_get_to_set(op_get);
            if (op_set == OP_RET && op_get != OP_CALL && op_get != OP_IMPORT && op_get != OP_ILIT) {
                compile_error_unexpected(c, &token);
            }

            compile_expr(c, POWER_SET);
            lexer_expect(&c->lexer, TOKEN_RBRACKET);

            chunk_push_op_loc(c->vm, c->chunk, loc);
            chunk_push_op(c->vm, c->chunk, OP_IGET);
        } break;

        case TOKEN_JOIN:
            compile_expr(c, lbp);
            chunk_push_op(c->vm, c->chunk, OP_JOIN);
            break;

        case TOKEN_SET: {
            const Op op = op_get_to_set(c->chunk->data[c->chunk->last]);
            if (op == OP_RET) {
                compile_error_unexpected(c, &token);
            }

            size_t index;
            if (op != OP_ISET) {
                index = *(size_t *)&c->chunk->data[c->chunk->last + 1];
            }
            c->chunk->count = c->chunk->last;

            compile_expr(c, lbp);

            chunk_push_op_loc(c->vm, c->chunk, loc);
            if (op == OP_ISET) {
                chunk_push_op(c->vm, c->chunk, op);
            } else {
                chunk_push_op_int(c->vm, c->chunk, op, index);
            }
        } break;

        default:
            assert(false && "unreachable");
        }
    }
}

static void compile_block_init(Compiler *c) {
    c->scope->depth++;
}

static void compile_block_end(Compiler *c) {
    c->scope->depth--;

    while (c->scope->count && c->scope->data[c->scope->count - 1].depth > c->scope->depth) {
        if (c->scope->data[c->scope->count - 1].captured) {
            chunk_push_op(c->vm, c->chunk, OP_UCLOSE);
        } else {
            chunk_push_op(c->vm, c->chunk, OP_DROP);
        }

        c->scope->count--;
    }
}

static void compile_scope_init(Compiler *c, Scope *scope, SV name) {
    scope->fn = object_fn_new(c->vm);
    scope->outer = c->scope;
    c->scope = scope;

    if (name.size) {
        scope->fn->name = object_str_const(c->vm, name.data, name.size);
    }

    c->chunk = &c->scope->fn->chunk;
    scope_push(c->vm, scope, (Local){0});
}

static ObjectFn *compile_scope_end(Compiler *c) {
    chunk_push_op(c->vm, c->chunk, OP_NIL);
    chunk_push_op(c->vm, c->chunk, OP_RET);
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

static size_t compile_definition(Compiler *c, Token *name) {
    *name = lexer_expect(&c->lexer, TOKEN_IDENT);

    if (c->scope->depth) {
        da_push(c->vm, c->scope, ((Local){.token = *name, .depth = c->scope->depth}));
        return 0;
    }

    values_push(
        c->vm,
        &c->chunk->constants,
        value_object(object_str_const(c->vm, name->sv.data, name->sv.size)));

    return c->chunk->constants.count - 1;
}

static void compile_stmt(Compiler *c);

static void compile_function(Compiler *c, const Token *name) {
    Scope scope = {0};
    compile_scope_init(c, &scope, name ? name->sv : (SV){0});
    compile_block_init(c);

    lexer_expect(&c->lexer, TOKEN_LPAREN);
    while (!lexer_read(&c->lexer, TOKEN_RPAREN)) {
        c->scope->fn->arity++;

        const Token arg = lexer_expect(&c->lexer, TOKEN_IDENT);
        da_push(c->vm, c->scope, ((Local){.token = arg, .depth = c->scope->depth}));

        const Token token = lexer_peek(&c->lexer);
        if (token.type != TOKEN_COMMA) {
            lexer_expect(&c->lexer, TOKEN_RPAREN);
            break;
        }
        c->lexer.peeked = false;
    }

    lexer_buffer(&c->lexer, lexer_expect(&c->lexer, TOKEN_LBRACE));
    compile_stmt(c);

    const ObjectFn *fn = compile_scope_end(c);
    chunk_push_op_value(c->vm, c->chunk, OP_CLOSURE, value_object(fn));

    for (size_t i = 0; i < scope.fn->upvalues; i++) {
        chunk_push_op_int(
            c->vm, c->chunk, scope.upvalues.data[i].local ? 1 : 0, scope.upvalues.data[i].index);
    }

    scope_free(c->vm, &scope);
}

static_assert(COUNT_TOKENS == 41, "Update compile_stmt()");
static void compile_stmt(Compiler *c) {
    Token token = lexer_next(&c->lexer);

    switch (token.type) {
    case TOKEN_LBRACE:
        compile_block_init(c);
        while (!lexer_read(&c->lexer, TOKEN_RBRACE)) {
            compile_stmt(c);
        }
        compile_block_end(c);
        break;

    case TOKEN_IF: {
        compile_expr(c, POWER_SET);

        const size_t then_addr = compile_jump_start(c, OP_ELSE);
        chunk_push_op(c->vm, c->chunk, OP_DROP);

        lexer_buffer(&c->lexer, lexer_expect(&c->lexer, TOKEN_LBRACE));
        compile_stmt(c);

        const size_t else_addr = compile_jump_start(c, OP_JUMP);
        compile_jump_patch(c, then_addr);
        chunk_push_op(c->vm, c->chunk, OP_DROP);

        if (lexer_read(&c->lexer, TOKEN_ELSE)) {
            lexer_buffer(&c->lexer, lexer_expect(&c->lexer, TOKEN_LBRACE));
            compile_stmt(c);
        }
        compile_jump_patch(c, else_addr);
    } break;

    case TOKEN_FOR: {
        compile_block_init(c);

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
        chunk_push_op(c->vm, c->chunk, OP_DROP);

        const size_t iter_addr = compile_jump_start(c, OP_JUMP);
        compile_expr(c, POWER_NIL);
        chunk_push_op(c->vm, c->chunk, OP_DROP);
        compile_jump_direct(c, OP_JUMP, cond_addr);
        compile_jump_patch(c, iter_addr);

        lexer_buffer(&c->lexer, lexer_expect(&c->lexer, TOKEN_LBRACE));
        compile_stmt(c);
        compile_jump_direct(c, OP_JUMP, iter_addr + 1 + sizeof(size_t));

        compile_jump_direct(c, OP_JUMP, cond_addr);
        compile_jump_patch(c, loop_addr);
        chunk_push_op(c->vm, c->chunk, OP_DROP);

        compile_block_end(c);
    } break;

    case TOKEN_WHILE: {
        const size_t cond_addr = c->chunk->count;
        compile_expr(c, POWER_SET);

        const size_t loop_addr = compile_jump_start(c, OP_ELSE);
        chunk_push_op(c->vm, c->chunk, OP_DROP);

        lexer_buffer(&c->lexer, lexer_expect(&c->lexer, TOKEN_LBRACE));
        compile_stmt(c);

        compile_jump_direct(c, OP_JUMP, cond_addr);
        compile_jump_patch(c, loop_addr);
        chunk_push_op(c->vm, c->chunk, OP_DROP);
    } break;

    case TOKEN_FN: {
        const size_t const_index = compile_definition(c, &token);
        compile_function(c, &token);

        if (!c->scope->depth) {
            chunk_push_op_int(c->vm, c->chunk, OP_GDEF, const_index);
        }
    } break;

    case TOKEN_VAR: {
        const size_t scope_index = c->scope->count;
        const size_t const_index = compile_definition(c, &token);

        if (c->scope->depth) {
            c->scope->data[scope_index].token = (Token){0};
        }

        if (lexer_read(&c->lexer, TOKEN_SET)) {
            compile_expr(c, POWER_SET);
        } else {
            chunk_push_op(c->vm, c->chunk, OP_NIL);
        }
        lexer_expect(&c->lexer, TOKEN_EOL);

        if (c->scope->depth) {
            c->scope->data[scope_index].token = token;
        } else {
            chunk_push_op_int(c->vm, c->chunk, OP_GDEF, const_index);
        }
    } break;

    case TOKEN_RETURN:
        token = lexer_peek(&c->lexer);

        if (token.type == TOKEN_EOL) {
            chunk_push_op(c->vm, c->chunk, OP_NIL);
        } else {
            compile_expr(c, POWER_SET);
        }

        lexer_expect(&c->lexer, TOKEN_EOL);
        chunk_push_op(c->vm, c->chunk, OP_RET);
        break;

    case TOKEN_PRINT:
        compile_expr(c, POWER_SET);
        lexer_expect(&c->lexer, TOKEN_EOL);
        chunk_push_op(c->vm, c->chunk, OP_PRINT);
        break;

    default:
        lexer_buffer(&c->lexer, token);
        compile_expr(c, POWER_NIL);
        lexer_expect(&c->lexer, TOKEN_EOL);
        chunk_push_op(c->vm, c->chunk, OP_DROP);
    }
}

const ObjectFn *compile(Vm *vm, const char *path, SV sv) {
    Compiler compiler = {.vm = vm, .lexer = lexer_new(path, sv)};
    const SV path_sv = (SV){path, strlen(path)};
    if (sv_suffix(path_sv, SVStatic(".bsx"))) {
        compiler.lexer.extended = true;
    }

    Scope scope = {0};
    compile_scope_init(&compiler, &scope, path_sv);
    scope.fn->module = vm_modules_push(vm, scope.fn->name);

    if (setjmp(compiler.lexer.error)) {
        Scope *scope = compiler.scope;
        while (scope) {
            Scope *outer = scope->outer;
            scope_free(compiler.vm, scope);
            scope = outer;
        }

        compiler.scope = NULL;
        compiler.chunk = NULL;
        return NULL;
    }

    while (!lexer_read(&compiler.lexer, TOKEN_EOF)) {
        compile_stmt(&compiler);
    }

    ObjectFn *fn = compile_scope_end(&compiler);
    scope_free(compiler.vm, &scope);

    return fn;
}
