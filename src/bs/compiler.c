#include <stdio.h>

#include "bs/compiler.h"
#include "bs/lexer.h"

typedef enum {
    BS_POWER_NIL,
    BS_POWER_SET,
    BS_POWER_LOR,
    BS_POWER_CMP,
    BS_POWER_SHL,
    BS_POWER_ADD,
    BS_POWER_BOR,
    BS_POWER_MUL,
    BS_POWER_PRE,
    BS_POWER_DOT,
} Bs_Power;

static_assert(BS_COUNT_TOKENS == 55, "Update bs_token_type_power()");
static Bs_Power bs_token_type_power(Bs_Token_Type type) {
    switch (type) {
    case BS_TOKEN_DOT:
    case BS_TOKEN_LPAREN:
    case BS_TOKEN_LBRACKET:
        return BS_POWER_DOT;

    case BS_TOKEN_ADD:
    case BS_TOKEN_SUB:
        return BS_POWER_ADD;

    case BS_TOKEN_MUL:
    case BS_TOKEN_DIV:
        return BS_POWER_MUL;

    case BS_TOKEN_BOR:
    case BS_TOKEN_BAND:
    case BS_TOKEN_BXOR:
        return BS_POWER_BOR;

    case BS_TOKEN_LOR:
    case BS_TOKEN_LAND:
    case BS_TOKEN_LXOR:
        return BS_POWER_LOR;

    case BS_TOKEN_SHL:
    case BS_TOKEN_SHR:
        return BS_POWER_SHL;

    case BS_TOKEN_GT:
    case BS_TOKEN_GE:
    case BS_TOKEN_LT:
    case BS_TOKEN_LE:
    case BS_TOKEN_EQ:
    case BS_TOKEN_NE:
        return BS_POWER_CMP;

    case BS_TOKEN_JOIN:
        return BS_POWER_ADD;

    case BS_TOKEN_SET:
        return BS_POWER_SET;

    default:
        return BS_POWER_NIL;
    }
}

typedef struct {
    size_t *data;
    size_t count;
    size_t depth;
    size_t start;
    size_t capacity;
} Bs_Jumps;

#define bs_jumps_free bs_da_free
#define bs_jumps_push bs_da_push

typedef struct {
    Bs_Token token;
    size_t depth;
    bool captured;
} Bs_Local;

typedef struct {
    bool local;
    size_t index;
} Bs_Uplocal;

typedef struct {
    Bs_Uplocal *data;
    size_t count;
    size_t capacity;
} Bs_Uplocals;

#define bs_uplocals_free bs_da_free
#define bs_uplocals_push bs_da_push

typedef struct Bs_Scope Bs_Scope;

struct Bs_Scope {
    Bs_Scope *outer;

    Bs_Local *data;
    size_t count;
    size_t depth;
    size_t capacity;

    Bs_Fn *fn;
    Bs_Uplocals uplocals;

    bool is_repl;
};

#define bs_scope_push bs_da_push

static void bs_scope_free(Bs *bs, Bs_Scope *s) {
    if (s) {
        bs_uplocals_free(bs, &s->uplocals);
        bs_da_free(bs, s);
    }
}

static bool bs_scope_find_local(Bs_Scope *s, Bs_Sv name, size_t *index) {
    for (size_t i = s->count; i > 0; i--) {
        if (bs_sv_eq(s->data[i - 1].token.sv, name)) {
            *index = i - 1;
            return true;
        }
    }

    return false;
}

static void bs_scope_add_upvalue(Bs *bs, Bs_Scope *s, size_t *index, bool local) {
    for (size_t i = 0; i < s->fn->upvalues; i++) {
        const Bs_Uplocal *upvalue = &s->uplocals.data[i];
        if (upvalue->index == *index && upvalue->local == local) {
            *index = i;
            return;
        }
    }

    const Bs_Uplocal upvalue = {.local = local, .index = *index};
    bs_uplocals_push(bs, &s->uplocals, upvalue);

    *index = s->fn->upvalues++;
}

static bool bs_scope_find_upvalue(Bs *bs, Bs_Scope *s, Bs_Sv name, size_t *index) {
    if (!s->outer) {
        return false;
    }

    if (bs_scope_find_local(s->outer, name, index)) {
        s->outer->data[*index].captured = true;
        bs_scope_add_upvalue(bs, s, index, true);
        return true;
    }

    if (bs_scope_find_upvalue(bs, s->outer, name, index)) {
        bs_scope_add_upvalue(bs, s, index, false);
        return true;
    }

    return false;
}

typedef struct {
    Bs_Lexer lexer;
    Bs_Scope *scope;

    Bs *bs;
    Bs_Chunk *chunk;

    Bs_Jumps jumps;

    bool is_main;
} Bs_Compiler;

static size_t bs_compile_jump_start(Bs_Compiler *c, Bs_Op op) {
    bs_chunk_push_op_int(c->bs, c->chunk, op, 0);
    return c->chunk->count - 1 - sizeof(size_t);
}

static void bs_compile_jump_patch(Bs_Compiler *c, size_t addr) {
    *(size_t *)&c->chunk->data[addr + 1] = c->chunk->count - 1 - sizeof(size_t) - addr;
}

static void bs_compile_jump_direct(Bs_Compiler *c, Bs_Op op, size_t addr) {
    bs_chunk_push_op_int(c->bs, c->chunk, op, addr - c->chunk->count - 1 - sizeof(size_t));
}

static void bs_compile_error_unexpected(Bs_Compiler *c, const Bs_Token *token) {
    bs_fmt(
        c->lexer.error,
        Bs_Loc_Fmt "error: unexpected %s\n",
        Bs_Loc_Arg(token->loc),
        bs_token_type_name(token->type, c->lexer.extended));

    bs_lexer_error(&c->lexer);
}

static void bs_compile_lambda(Bs_Compiler *c, const Bs_Token *name);

static void bs_compile_string(Bs_Compiler *c, Bs_Sv sv) {
    Bs_Buffer *b = bs_buffer_get(c->bs);
    const size_t start = b->count;

    Bs_Writer w = bs_buffer_writer(b);
    for (size_t i = 0; i < sv.size; i++) {
        char ch = sv.data[i];
        if (ch == '\\') {
            switch (sv.data[++i]) {
            case 'n':
                ch = '\n';
                break;

            case 't':
                ch = '\t';
                break;

            case '0':
                ch = '\0';
                break;

            case '"':
                ch = '\"';
                break;

            case '\\':
                ch = '\\';
                break;
            }
        }

        bs_buffer_push(c->bs, b, ch);
    }

    bs_chunk_push_op_value(
        c->bs,
        c->chunk,
        BS_OP_CONST,
        bs_value_object(bs_str_const(c->bs, bs_buffer_reset(b, start))));
}

static_assert(BS_COUNT_TOKENS == 55, "Update bs_compile_expr()");
static void bs_compile_expr(Bs_Compiler *c, Bs_Power mbp) {
    Bs_Token token = bs_lexer_next(&c->lexer);
    Bs_Loc loc = token.loc;

    switch (token.type) {
    case BS_TOKEN_NIL:
        bs_chunk_push_op(c->bs, c->chunk, BS_OP_NIL);
        break;

    case BS_TOKEN_STR:
        bs_compile_string(c, token.sv);
        break;

    case BS_TOKEN_ISTR:
        bs_compile_string(c, token.sv);

        while (token.type == BS_TOKEN_ISTR) {
            bs_lexer_expect(&c->lexer, BS_TOKEN_LPAREN);
            bs_compile_expr(c, BS_POWER_SET);
            bs_lexer_expect(&c->lexer, BS_TOKEN_RPAREN);

            bs_chunk_push_op(c->bs, c->chunk, BS_OP_JOIN);

            token = bs_lexer_str(&c->lexer, c->lexer.loc);

            if (token.sv.size) {
                bs_compile_string(c, token.sv);
                bs_chunk_push_op(c->bs, c->chunk, BS_OP_JOIN);
            }
        }
        break;

    case BS_TOKEN_NUM:
        bs_chunk_push_op_value(
            c->bs, c->chunk, BS_OP_CONST, bs_value_num(strtod(token.sv.data, NULL)));
        break;

    case BS_TOKEN_TRUE:
        bs_chunk_push_op(c->bs, c->chunk, BS_OP_TRUE);
        break;

    case BS_TOKEN_FALSE:
        bs_chunk_push_op(c->bs, c->chunk, BS_OP_FALSE);
        break;

    case BS_TOKEN_IDENT: {
        size_t index;
        if (bs_scope_find_local(c->scope, token.sv, &index)) {
            bs_chunk_push_op_int(c->bs, c->chunk, BS_OP_LGET, index);
        } else if (bs_scope_find_upvalue(c->bs, c->scope, token.sv, &index)) {
            bs_chunk_push_op_int(c->bs, c->chunk, BS_OP_UGET, index);
        } else {
            bs_chunk_push_op_value(
                c->bs, c->chunk, BS_OP_GGET, bs_value_object(bs_str_const(c->bs, token.sv)));
            bs_chunk_push_op_loc(c->bs, c->chunk, loc);
        }
    } break;

    case BS_TOKEN_LPAREN:
        bs_compile_expr(c, BS_POWER_SET);
        bs_lexer_expect(&c->lexer, BS_TOKEN_RPAREN);
        break;

    case BS_TOKEN_LBRACE: {
        bs_chunk_push_op(c->bs, c->chunk, BS_OP_TABLE);

        while (!bs_lexer_read(&c->lexer, BS_TOKEN_RBRACE)) {
            token = bs_lexer_either(&c->lexer, BS_TOKEN_IDENT, BS_TOKEN_LBRACKET);
            loc = token.loc;

            if (token.type == BS_TOKEN_LBRACKET) {
                bs_compile_expr(c, BS_POWER_SET);
                bs_lexer_expect(&c->lexer, BS_TOKEN_RBRACKET);
            } else {
                bs_chunk_push_op_value(
                    c->bs, c->chunk, BS_OP_CONST, bs_value_object(bs_str_const(c->bs, token.sv)));
            }

            bs_lexer_expect(&c->lexer, BS_TOKEN_SET);
            bs_compile_expr(c, BS_POWER_SET);

            bs_chunk_push_op(c->bs, c->chunk, BS_OP_ILIT);
            bs_chunk_push_op_loc(c->bs, c->chunk, loc);

            if (bs_lexer_either(&c->lexer, BS_TOKEN_COMMA, BS_TOKEN_RBRACE).type !=
                BS_TOKEN_COMMA) {
                break;
            }
        }
    } break;

    case BS_TOKEN_LBRACKET: {
        bs_chunk_push_op(c->bs, c->chunk, BS_OP_ARRAY);

        size_t index = 0;
        while (!bs_lexer_read(&c->lexer, BS_TOKEN_RBRACKET)) {
            bs_chunk_push_op_value(c->bs, c->chunk, BS_OP_CONST, bs_value_num(index));
            bs_compile_expr(c, BS_POWER_SET);
            index++;

            bs_chunk_push_op(c->bs, c->chunk, BS_OP_ILIT);

            if (bs_lexer_either(&c->lexer, BS_TOKEN_COMMA, BS_TOKEN_RBRACKET).type !=
                BS_TOKEN_COMMA) {
                break;
            }
        }
    } break;

    case BS_TOKEN_SUB:
        bs_compile_expr(c, BS_POWER_PRE);
        bs_chunk_push_op(c->bs, c->chunk, BS_OP_NEG);
        bs_chunk_push_op_loc(c->bs, c->chunk, loc);
        break;

    case BS_TOKEN_BNOT:
        bs_compile_expr(c, BS_POWER_PRE);
        bs_chunk_push_op(c->bs, c->chunk, BS_OP_BNOT);
        bs_chunk_push_op_loc(c->bs, c->chunk, loc);
        break;

    case BS_TOKEN_LNOT:
        bs_compile_expr(c, BS_POWER_PRE);
        bs_chunk_push_op(c->bs, c->chunk, BS_OP_LNOT);
        break;

    case BS_TOKEN_LEN:
        bs_lexer_expect(&c->lexer, BS_TOKEN_LPAREN);
        bs_compile_expr(c, BS_POWER_PRE);
        bs_lexer_expect(&c->lexer, BS_TOKEN_RPAREN);

        bs_chunk_push_op(c->bs, c->chunk, BS_OP_LEN);
        bs_chunk_push_op_loc(c->bs, c->chunk, loc);
        break;

    case BS_TOKEN_IMPORT:
        bs_lexer_expect(&c->lexer, BS_TOKEN_LPAREN);
        bs_compile_expr(c, BS_POWER_SET);
        bs_lexer_expect(&c->lexer, BS_TOKEN_RPAREN);

        bs_chunk_push_op(c->bs, c->chunk, BS_OP_IMPORT);
        bs_chunk_push_op_loc(c->bs, c->chunk, loc);
        break;

    case BS_TOKEN_TYPEOF:
        bs_lexer_expect(&c->lexer, BS_TOKEN_LPAREN);
        bs_compile_expr(c, BS_POWER_SET);
        bs_lexer_expect(&c->lexer, BS_TOKEN_RPAREN);

        bs_chunk_push_op(c->bs, c->chunk, BS_OP_TYPEOF);
        break;

    case BS_TOKEN_IF: {
        bs_compile_expr(c, BS_POWER_SET);

        const size_t then_addr = bs_compile_jump_start(c, BS_OP_ELSE);
        bs_chunk_push_op(c->bs, c->chunk, BS_OP_DROP);

        bs_lexer_expect(&c->lexer, BS_TOKEN_THEN);
        bs_compile_expr(c, BS_POWER_SET);

        const size_t else_addr = bs_compile_jump_start(c, BS_OP_JUMP);
        bs_compile_jump_patch(c, then_addr);
        bs_chunk_push_op(c->bs, c->chunk, BS_OP_DROP);

        bs_lexer_expect(&c->lexer, BS_TOKEN_ELSE);
        bs_compile_expr(c, BS_POWER_SET);
        bs_compile_jump_patch(c, else_addr);
    } break;

    case BS_TOKEN_FN:
        bs_compile_lambda(c, NULL);
        break;

    case BS_TOKEN_IS_MAIN_MODULE:
        bs_chunk_push_op(c->bs, c->chunk, c->is_main ? BS_OP_TRUE : BS_OP_FALSE);
        break;

    default:
        bs_compile_error_unexpected(c, &token);
        return;
    }

    while (true) {
        token = bs_lexer_peek(&c->lexer);

        const Bs_Power lbp = bs_token_type_power(token.type);
        if (lbp <= mbp) {
            break;
        }
        c->lexer.peeked = false;

        loc = token.loc;
        switch (token.type) {
        case BS_TOKEN_DOT: {
            const Bs_Op op_get = c->chunk->data[c->chunk->last];
            const Bs_Op op_set = bs_op_get_to_set(op_get);
            if (op_set == BS_OP_RET && op_get != BS_OP_CALL && op_get != BS_OP_IMPORT &&
                op_get != BS_OP_ILIT) {
                bs_compile_error_unexpected(c, &token);
            }

            token = bs_lexer_expect(&c->lexer, BS_TOKEN_IDENT);
            bs_chunk_push_op_value(
                c->bs, c->chunk, BS_OP_CONST, bs_value_object(bs_str_const(c->bs, token.sv)));

            bs_chunk_push_op(c->bs, c->chunk, BS_OP_IGET);
            bs_chunk_push_op_loc(c->bs, c->chunk, loc);
        } break;

        case BS_TOKEN_ADD:
            bs_compile_expr(c, lbp);
            bs_chunk_push_op(c->bs, c->chunk, BS_OP_ADD);
            bs_chunk_push_op_loc(c->bs, c->chunk, loc);
            break;

        case BS_TOKEN_SUB:
            bs_compile_expr(c, lbp);
            bs_chunk_push_op(c->bs, c->chunk, BS_OP_SUB);
            bs_chunk_push_op_loc(c->bs, c->chunk, loc);
            break;

        case BS_TOKEN_MUL:
            bs_compile_expr(c, lbp);
            bs_chunk_push_op(c->bs, c->chunk, BS_OP_MUL);
            bs_chunk_push_op_loc(c->bs, c->chunk, loc);
            break;

        case BS_TOKEN_DIV:
            bs_compile_expr(c, lbp);
            bs_chunk_push_op(c->bs, c->chunk, BS_OP_DIV);
            bs_chunk_push_op_loc(c->bs, c->chunk, loc);
            break;

        case BS_TOKEN_BOR:
            bs_compile_expr(c, lbp);
            bs_chunk_push_op(c->bs, c->chunk, BS_OP_BOR);
            bs_chunk_push_op_loc(c->bs, c->chunk, loc);
            break;

        case BS_TOKEN_BAND:
            bs_compile_expr(c, lbp);
            bs_chunk_push_op(c->bs, c->chunk, BS_OP_BAND);
            bs_chunk_push_op_loc(c->bs, c->chunk, loc);
            break;

        case BS_TOKEN_BXOR:
            bs_compile_expr(c, lbp);
            bs_chunk_push_op(c->bs, c->chunk, BS_OP_BXOR);
            bs_chunk_push_op_loc(c->bs, c->chunk, loc);
            break;

        case BS_TOKEN_LOR: {
            const size_t addr = bs_compile_jump_start(c, BS_OP_THEN);

            bs_chunk_push_op(c->bs, c->chunk, BS_OP_DROP);
            bs_compile_expr(c, lbp);

            bs_compile_jump_patch(c, addr);
        } break;

        case BS_TOKEN_LAND: {
            const size_t addr = bs_compile_jump_start(c, BS_OP_ELSE);

            bs_chunk_push_op(c->bs, c->chunk, BS_OP_DROP);
            bs_compile_expr(c, lbp);

            bs_compile_jump_patch(c, addr);
        } break;

        case BS_TOKEN_LXOR:
            bs_compile_expr(c, lbp);
            bs_chunk_push_op(c->bs, c->chunk, BS_OP_LXOR);
            bs_chunk_push_op_loc(c->bs, c->chunk, loc);
            break;

        case BS_TOKEN_SHL:
            bs_compile_expr(c, lbp);
            bs_chunk_push_op(c->bs, c->chunk, BS_OP_SHL);
            bs_chunk_push_op_loc(c->bs, c->chunk, loc);
            break;

        case BS_TOKEN_SHR:
            bs_compile_expr(c, lbp);
            bs_chunk_push_op(c->bs, c->chunk, BS_OP_SHR);
            bs_chunk_push_op_loc(c->bs, c->chunk, loc);
            break;

        case BS_TOKEN_GT:
            bs_compile_expr(c, lbp);
            bs_chunk_push_op(c->bs, c->chunk, BS_OP_GT);
            bs_chunk_push_op_loc(c->bs, c->chunk, loc);
            break;

        case BS_TOKEN_GE:
            bs_compile_expr(c, lbp);
            bs_chunk_push_op(c->bs, c->chunk, BS_OP_GE);
            bs_chunk_push_op_loc(c->bs, c->chunk, loc);
            break;

        case BS_TOKEN_LT:
            bs_compile_expr(c, lbp);
            bs_chunk_push_op(c->bs, c->chunk, BS_OP_LT);
            bs_chunk_push_op_loc(c->bs, c->chunk, loc);
            break;

        case BS_TOKEN_LE:
            bs_compile_expr(c, lbp);
            bs_chunk_push_op(c->bs, c->chunk, BS_OP_LE);
            bs_chunk_push_op_loc(c->bs, c->chunk, loc);
            break;

        case BS_TOKEN_EQ:
            bs_compile_expr(c, lbp);
            bs_chunk_push_op(c->bs, c->chunk, BS_OP_EQ);
            bs_chunk_push_op_loc(c->bs, c->chunk, loc);
            break;

        case BS_TOKEN_NE:
            bs_compile_expr(c, lbp);
            bs_chunk_push_op(c->bs, c->chunk, BS_OP_NE);
            bs_chunk_push_op_loc(c->bs, c->chunk, loc);
            break;

        case BS_TOKEN_LPAREN: {
            const Bs_Op op_get = c->chunk->data[c->chunk->last];
            const Bs_Op op_set = bs_op_get_to_set(op_get);
            if (op_set == BS_OP_RET && op_get != BS_OP_CALL && op_get != BS_OP_IMPORT &&
                op_get != BS_OP_CLOSURE) {
                bs_compile_error_unexpected(c, &token);
            }

            size_t arity = 0;
            while (!bs_lexer_read(&c->lexer, BS_TOKEN_RPAREN)) {
                bs_compile_expr(c, BS_POWER_SET);
                arity++;

                if (bs_lexer_either(&c->lexer, BS_TOKEN_COMMA, BS_TOKEN_RPAREN).type !=
                    BS_TOKEN_COMMA) {
                    break;
                }
            }

            bs_chunk_push_op_int(c->bs, c->chunk, BS_OP_CALL, arity);
            bs_chunk_push_op_loc(c->bs, c->chunk, loc);
        } break;

        case BS_TOKEN_LBRACKET: {
            const Bs_Op op_get = c->chunk->data[c->chunk->last];
            const Bs_Op op_set = bs_op_get_to_set(op_get);
            if (op_set == BS_OP_RET && op_get != BS_OP_CALL && op_get != BS_OP_IMPORT &&
                op_get != BS_OP_ILIT) {
                bs_compile_error_unexpected(c, &token);
            }

            bs_compile_expr(c, BS_POWER_SET);
            bs_lexer_expect(&c->lexer, BS_TOKEN_RBRACKET);

            bs_chunk_push_op(c->bs, c->chunk, BS_OP_IGET);
            bs_chunk_push_op_loc(c->bs, c->chunk, loc);
        } break;

        case BS_TOKEN_JOIN:
            bs_compile_expr(c, lbp);
            bs_chunk_push_op(c->bs, c->chunk, BS_OP_JOIN);
            break;

        case BS_TOKEN_SET: {
            const Bs_Op op = bs_op_get_to_set(c->chunk->data[c->chunk->last]);
            if (op == BS_OP_RET) {
                bs_compile_error_unexpected(c, &token);
            }

            size_t index;
            if (op != BS_OP_ISET) {
                index = *(size_t *)&c->chunk->data[c->chunk->last + 1];
            }
            c->chunk->count = c->chunk->last;

            bs_compile_expr(c, lbp);

            if (op == BS_OP_ISET) {
                bs_chunk_push_op(c->bs, c->chunk, op);
            } else {
                bs_chunk_push_op_int(c->bs, c->chunk, op, index);
            }
            bs_chunk_push_op_loc(c->bs, c->chunk, loc);
        } break;

        default:
            assert(false && "unreachable");
        }
    }
}

static void bs_compile_block_init(Bs_Compiler *c) {
    c->scope->depth++;
}

static size_t bs_compile_block_drop(Bs_Compiler *c, size_t depth) {
    size_t scope_count = c->scope->count;
    while (scope_count && c->scope->data[scope_count - 1].depth > depth) {
        if (c->scope->data[scope_count - 1].captured) {
            bs_chunk_push_op(c->bs, c->chunk, BS_OP_UCLOSE);
        } else {
            bs_chunk_push_op(c->bs, c->chunk, BS_OP_DROP);
        }

        scope_count--;
    }

    return scope_count;
}

static void bs_compile_block_end(Bs_Compiler *c) {
    c->scope->count = bs_compile_block_drop(c, --c->scope->depth);
}

static void bs_compile_scope_init(Bs_Compiler *c, Bs_Scope *scope, Bs_Sv name) {
    scope->fn = bs_fn_new(c->bs);
    scope->outer = c->scope;
    c->scope = scope;

    if (name.size) {
        scope->fn->name = bs_str_const(c->bs, name);
    }

    c->chunk = &c->scope->fn->chunk;
    bs_scope_push(c->bs, scope, (Bs_Local){0});
}

static Bs_Fn *bs_compile_scope_end(Bs_Compiler *c) {
    bs_chunk_push_op(c->bs, c->chunk, BS_OP_NIL);
    bs_chunk_push_op(c->bs, c->chunk, BS_OP_RET);
    Bs_Fn *fn = c->scope->fn;

    Bs_Scope *outer = c->scope->outer;
    c->scope = outer;

    if (c->scope) {
        c->chunk = &c->scope->fn->chunk;
    } else {
        c->chunk = NULL;
    }

    fn->extended = c->lexer.extended;
    return fn;
}

static size_t bs_compile_definition(Bs_Compiler *c, Bs_Token *name, bool public) {
    *name = bs_lexer_expect(&c->lexer, BS_TOKEN_IDENT);

    if (public) {
        bs_values_push(c->bs, &c->chunk->constants, bs_value_object(bs_str_const(c->bs, name->sv)));
        return c->chunk->constants.count - 1;
    }

    bs_da_push(c->bs, c->scope, ((Bs_Local){.token = *name, .depth = c->scope->depth}));
    return 0;
}

static void bs_compile_stmt(Bs_Compiler *c);

static void bs_compile_lambda(Bs_Compiler *c, const Bs_Token *name) {
    Bs_Scope scope = {0};
    bs_compile_scope_init(c, &scope, name ? name->sv : (Bs_Sv){0});
    bs_compile_block_init(c);

    bs_lexer_expect(&c->lexer, BS_TOKEN_LPAREN);
    while (!bs_lexer_read(&c->lexer, BS_TOKEN_RPAREN)) {
        c->scope->fn->arity++;

        const Bs_Token arg = bs_lexer_expect(&c->lexer, BS_TOKEN_IDENT);
        bs_da_push(c->bs, c->scope, ((Bs_Local){.token = arg, .depth = c->scope->depth}));

        if (bs_lexer_either(&c->lexer, BS_TOKEN_COMMA, BS_TOKEN_RPAREN).type != BS_TOKEN_COMMA) {
            break;
        }
    }

    bs_lexer_buffer(&c->lexer, bs_lexer_expect(&c->lexer, BS_TOKEN_LBRACE));
    bs_compile_stmt(c);

    const Bs_Fn *fn = bs_compile_scope_end(c);
    bs_chunk_push_op_value(c->bs, c->chunk, BS_OP_CLOSURE, bs_value_object(fn));

    for (size_t i = 0; i < scope.fn->upvalues; i++) {
        bs_chunk_push_op_int(
            c->bs, c->chunk, scope.uplocals.data[i].local ? 1 : 0, scope.uplocals.data[i].index);
    }

    bs_scope_free(c->bs, &scope);
}

static void bs_compile_function(Bs_Compiler *c, bool public) {
    Bs_Token token;

    const size_t const_index = bs_compile_definition(c, &token, public);
    bs_compile_lambda(c, &token);

    if (public) {
        bs_chunk_push_op_int(c->bs, c->chunk, BS_OP_GDEF, const_index);
        bs_chunk_push_op_loc(c->bs, c->chunk, token.loc);
    }
}

static void bs_compile_variable(Bs_Compiler *c, bool public) {
    Bs_Token token;

    const size_t scope_index = c->scope->count;
    const size_t const_index = bs_compile_definition(c, &token, public);

    if (!public) {
        c->scope->data[scope_index].token = (Bs_Token){0};
    }

    if (bs_lexer_read(&c->lexer, BS_TOKEN_SET)) {
        bs_compile_expr(c, BS_POWER_SET);
    } else {
        bs_chunk_push_op(c->bs, c->chunk, BS_OP_NIL);
    }
    bs_lexer_expect(&c->lexer, BS_TOKEN_EOL);

    if (public) {
        bs_chunk_push_op_int(c->bs, c->chunk, BS_OP_GDEF, const_index);
        bs_chunk_push_op_loc(c->bs, c->chunk, token.loc);
    } else {
        c->scope->data[scope_index].token = token;
    }
}

static Bs_Jumps bs_compile_jumps_save(Bs_Compiler *c, size_t start) {
    const Bs_Jumps save = c->jumps;
    c->jumps.depth = c->scope->depth;
    c->jumps.start = start;
    return save;
}

static void bs_compile_jumps_reset(Bs_Compiler *c, Bs_Jumps save) {
    for (size_t i = save.count; i < c->jumps.count; i++) {
        bs_compile_jump_patch(c, c->jumps.data[i]);
    }

    c->jumps.count = save.count;
    c->jumps.depth = save.depth;
    c->jumps.start = save.start;
}

static_assert(BS_COUNT_TOKENS == 55, "Update bs_compile_stmt()");
static void bs_compile_stmt(Bs_Compiler *c) {
    Bs_Token token = bs_lexer_next(&c->lexer);

    switch (token.type) {
    case BS_TOKEN_LBRACE:
        bs_compile_block_init(c);
        while (!bs_lexer_read(&c->lexer, BS_TOKEN_RBRACE)) {
            bs_compile_stmt(c);
        }
        bs_compile_block_end(c);
        break;

    case BS_TOKEN_IF: {
        bs_compile_expr(c, BS_POWER_SET);

        const size_t then_addr = bs_compile_jump_start(c, BS_OP_ELSE);
        bs_chunk_push_op(c->bs, c->chunk, BS_OP_DROP);

        bs_lexer_buffer(&c->lexer, bs_lexer_expect(&c->lexer, BS_TOKEN_LBRACE));
        bs_compile_stmt(c);

        const size_t else_addr = bs_compile_jump_start(c, BS_OP_JUMP);
        bs_compile_jump_patch(c, then_addr);
        bs_chunk_push_op(c->bs, c->chunk, BS_OP_DROP);

        if (bs_lexer_read(&c->lexer, BS_TOKEN_ELSE)) {
            bs_lexer_buffer(&c->lexer, bs_lexer_either(&c->lexer, BS_TOKEN_LBRACE, BS_TOKEN_IF));
            bs_compile_stmt(c);
        }
        bs_compile_jump_patch(c, else_addr);
    } break;

    case BS_TOKEN_FOR: {
        bs_compile_block_init(c);

        const Bs_Token a = bs_lexer_expect(&c->lexer, BS_TOKEN_IDENT);
        Bs_Token b = bs_lexer_either(&c->lexer, BS_TOKEN_COMMA, BS_TOKEN_IN);
        Bs_Loc loc = b.loc;

        if (b.type != BS_TOKEN_IN) {
            b = bs_lexer_expect(&c->lexer, BS_TOKEN_IDENT);
            loc = bs_lexer_expect(&c->lexer, BS_TOKEN_IN).loc;
        }

        // Container / Start
        bs_compile_expr(c, BS_POWER_SET);
        bs_scope_push(c->bs, c->scope, (Bs_Local){.depth = c->scope->depth});

        if (b.type == BS_TOKEN_IN) {
            // End
            bs_lexer_expect(&c->lexer, BS_TOKEN_COMMA);
            bs_compile_expr(c, BS_POWER_SET);
            bs_scope_push(c->bs, c->scope, (Bs_Local){.depth = c->scope->depth});

            // Step
            if (bs_lexer_read(&c->lexer, BS_TOKEN_COMMA)) {
                bs_compile_expr(c, BS_POWER_SET);
            } else {
                bs_chunk_push_op(c->bs, c->chunk, BS_OP_NIL);
            }
        } else {
            // Iterator
            bs_chunk_push_op(c->bs, c->chunk, BS_OP_NIL);
        }
        bs_scope_push(c->bs, c->scope, (Bs_Local){.depth = c->scope->depth});

        const Bs_Jumps jumps_save = bs_compile_jumps_save(c, c->chunk->count);
        bs_compile_block_init(c);

        // Key
        bs_da_push(c->bs, c->scope, ((Bs_Local){.token = a, .depth = c->scope->depth}));

        if (b.type != BS_TOKEN_IN) {
            // Value
            bs_da_push(c->bs, c->scope, ((Bs_Local){.token = b, .depth = c->scope->depth}));
        }

        const size_t loop_addr =
            bs_compile_jump_start(c, b.type == BS_TOKEN_IN ? BS_OP_RANGE : BS_OP_ITER);

        bs_chunk_push_op_loc(c->bs, c->chunk, loc);

        bs_lexer_buffer(&c->lexer, bs_lexer_expect(&c->lexer, BS_TOKEN_LBRACE));
        bs_compile_stmt(c);

        bs_compile_block_end(c);

        bs_compile_jump_direct(c, BS_OP_JUMP, loop_addr);
        bs_compile_jump_patch(c, loop_addr);

        bs_compile_jumps_reset(c, jumps_save);
        bs_compile_block_end(c);
    } break;

    case BS_TOKEN_WHILE: {
        const size_t cond_addr = c->chunk->count;
        bs_compile_expr(c, BS_POWER_SET);

        const size_t loop_addr = bs_compile_jump_start(c, BS_OP_ELSE);
        bs_chunk_push_op(c->bs, c->chunk, BS_OP_DROP);

        const Bs_Jumps jumps_save = bs_compile_jumps_save(c, cond_addr);
        bs_lexer_buffer(&c->lexer, bs_lexer_expect(&c->lexer, BS_TOKEN_LBRACE));
        bs_compile_stmt(c);

        bs_compile_jump_direct(c, BS_OP_JUMP, cond_addr);
        bs_compile_jump_patch(c, loop_addr);
        bs_chunk_push_op(c->bs, c->chunk, BS_OP_DROP);

        bs_compile_jumps_reset(c, jumps_save);
    } break;

    case BS_TOKEN_BREAK: {
        if (!c->jumps.depth) {
            bs_compile_error_unexpected(c, &token);
        }
        bs_lexer_expect(&c->lexer, BS_TOKEN_EOL);

        bs_compile_block_drop(c, c->jumps.depth);
        bs_jumps_push(c->bs, &c->jumps, c->chunk->count);
        bs_chunk_push_op_int(c->bs, c->chunk, BS_OP_JUMP, 0);
    } break;

    case BS_TOKEN_CONTINUE: {
        if (!c->jumps.depth) {
            bs_compile_error_unexpected(c, &token);
        }
        bs_lexer_expect(&c->lexer, BS_TOKEN_EOL);

        bs_compile_block_drop(c, c->jumps.depth);
        bs_compile_jump_direct(c, BS_OP_JUMP, c->jumps.start);
    } break;

    case BS_TOKEN_FN:
        bs_compile_function(c, c->scope->is_repl && c->scope->depth == 1);
        break;

    case BS_TOKEN_PUB:
        if (c->scope->depth != 1) {
            bs_fmt(
                c->lexer.error,
                Bs_Loc_Fmt "error: cannot define public values in local scope\n",
                Bs_Loc_Arg(token.loc));

            bs_lexer_error(&c->lexer);
        }

        token = bs_lexer_next(&c->lexer);
        if (token.type == BS_TOKEN_FN) {
            bs_compile_function(c, true);
        } else if (token.type == BS_TOKEN_VAR) {
            bs_compile_variable(c, true);
        } else {
            bs_fmt(
                c->lexer.error,
                Bs_Loc_Fmt "error: expected 'fn' or 'var', got %s\n",
                Bs_Loc_Arg(token.loc),
                bs_token_type_name(token.type, c->lexer.extended));

            bs_lexer_error(&c->lexer);
        }
        break;

    case BS_TOKEN_VAR:
        bs_compile_variable(c, c->scope->is_repl && c->scope->depth == 1);
        break;

    case BS_TOKEN_RETURN:
        token = bs_lexer_peek(&c->lexer);

        if (token.type == BS_TOKEN_EOL) {
            bs_chunk_push_op(c->bs, c->chunk, BS_OP_NIL);
        } else {
            bs_compile_expr(c, BS_POWER_SET);
        }

        bs_lexer_expect(&c->lexer, BS_TOKEN_EOL);
        bs_chunk_push_op(c->bs, c->chunk, BS_OP_RET);
        break;

    default:
        bs_lexer_buffer(&c->lexer, token);
        bs_compile_expr(c, BS_POWER_NIL);
        bs_lexer_expect(&c->lexer, BS_TOKEN_EOL);
        bs_chunk_push_op(c->bs, c->chunk, BS_OP_DROP);
    }
}

Bs_Fn *bs_compile(Bs *bs, const char *path, Bs_Sv input, bool is_main, bool is_repl) {
    Bs_Compiler compiler = {
        .bs = bs,
        .lexer = bs_lexer_new(path, input, bs_stderr_get(bs)),
        .is_main = is_main,
    };

    Bs_Sv name = bs_sv_from_cstr(path);
    compiler.lexer.extended = bs_sv_suffix(name, Bs_Sv_Static(".bsx"));

    for (size_t i = name.size; i > 0; i--) {
        if (name.data[i - 1] == '.') {
            name.size = i - 1;
            break;
        }
    }

    Bs_Scope scope = {.is_repl = is_repl};
    bs_compile_scope_init(&compiler, &scope, name);
    bs_compile_block_init(&compiler);

    if (setjmp(compiler.lexer.unwind)) {
        Bs_Scope *scope = compiler.scope;
        while (scope) {
            Bs_Scope *outer = scope->outer;
            bs_scope_free(compiler.bs, scope);
            scope = outer;
        }

        compiler.scope = NULL;
        compiler.chunk = NULL;

        bs_jumps_free(compiler.bs, &compiler.jumps);
        return NULL;
    }

    while (!bs_lexer_read(&compiler.lexer, BS_TOKEN_EOF)) {
        bs_compile_stmt(&compiler);
    }

    if (is_repl && compiler.chunk->last < compiler.chunk->count &&
        compiler.chunk->data[compiler.chunk->last] == BS_OP_DROP) {
        compiler.chunk->data[compiler.chunk->last] = BS_OP_RET;
    }

    Bs_Fn *fn = bs_compile_scope_end(&compiler);
    bs_scope_free(compiler.bs, &scope);
    bs_jumps_free(compiler.bs, &compiler.jumps);
    return fn;
}
