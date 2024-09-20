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

static_assert(BS_COUNT_TOKENS == 59, "Update bs_token_type_power()");
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
    case BS_TOKEN_MOD:
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
    Bs_Sv name;
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

typedef struct Bs_Class_Compiler Bs_Class_Compiler;

struct Bs_Class_Compiler {
    Bs_Class_Compiler *outer;
};

typedef enum {
    BS_LAMBDA_FN,
    BS_LAMBDA_METHOD,
} Bs_Lambda_Type;

typedef struct Bs_Lambda Bs_Lambda;

struct Bs_Lambda {
    Bs_Lambda_Type type;
    Bs_Lambda *outer;

    Bs_Local *data;
    size_t count;
    size_t depth;
    size_t capacity;

    Bs_Fn *fn;
    Bs_Uplocals uplocals;
};

#define bs_lambda_push bs_da_push

static void bs_lambda_free(Bs *bs, Bs_Lambda *l) {
    if (l) {
        bs_uplocals_free(bs, &l->uplocals);
        bs_da_free(bs, l);
    }
}

static bool bs_lambda_find_local(Bs_Lambda *l, Bs_Sv name, size_t *index) {
    for (size_t i = l->count; i > 0; i--) {
        if (bs_sv_eq(l->data[i - 1].name, name)) {
            *index = i - 1;
            return true;
        }
    }

    return false;
}

static void bs_lambda_add_upvalue(Bs *bs, Bs_Lambda *l, size_t *index, bool local) {
    for (size_t i = 0; i < l->fn->upvalues; i++) {
        const Bs_Uplocal *upvalue = &l->uplocals.data[i];
        if (upvalue->index == *index && upvalue->local == local) {
            *index = i;
            return;
        }
    }

    const Bs_Uplocal upvalue = {.local = local, .index = *index};
    bs_uplocals_push(bs, &l->uplocals, upvalue);

    *index = l->fn->upvalues++;
}

static bool bs_lambda_find_upvalue(Bs *bs, Bs_Lambda *l, Bs_Sv name, size_t *index) {
    if (!l->outer) {
        return false;
    }

    if (bs_lambda_find_local(l->outer, name, index)) {
        l->outer->data[*index].captured = true;
        bs_lambda_add_upvalue(bs, l, index, true);
        return true;
    }

    if (bs_lambda_find_upvalue(bs, l->outer, name, index)) {
        bs_lambda_add_upvalue(bs, l, index, false);
        return true;
    }

    return false;
}

typedef struct {
    Bs_Lexer lexer;
    Bs_Lambda *lambda;
    Bs_Class_Compiler *class;

    Bs *bs;
    Bs_Chunk *chunk;

    Bs_Jumps jumps;
    Bs_Op_Locs locations;

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

static void bs_compile_lambda(Bs_Compiler *c, Bs_Lambda_Type type, const Bs_Token *name);

static void bs_compile_string(Bs_Compiler *c, Bs_Sv sv) {
    Bs_Buffer *b = &bs_config(c->bs)->buffer;
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

        bs_da_push(c->bs, b, ch);
    }

    bs_chunk_push_op_value(
        c->bs,
        c->chunk,
        BS_OP_CONST,
        bs_value_object(bs_str_new(c->bs, bs_buffer_reset(b, start))));
}

static void bs_compile_identifier(Bs_Compiler *c, const Bs_Token *token) {
    size_t index;
    if (bs_lambda_find_local(c->lambda, token->sv, &index)) {
        bs_chunk_push_op_int(c->bs, c->chunk, BS_OP_LGET, index);
    } else if (bs_lambda_find_upvalue(c->bs, c->lambda, token->sv, &index)) {
        bs_chunk_push_op_int(c->bs, c->chunk, BS_OP_UGET, index);
    } else {
        bs_chunk_push_op_value(
            c->bs, c->chunk, BS_OP_GGET, bs_value_object(bs_str_new(c->bs, token->sv)));
    }
    bs_chunk_push_op_loc(c->bs, c->chunk, token->loc);
}

static_assert(BS_COUNT_TOKENS == 59, "Update bs_compile_expr()");
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

    case BS_TOKEN_IDENT:
        bs_compile_identifier(c, &token);
        break;

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
                loc = bs_lexer_peek(&c->lexer).loc;
                bs_compile_expr(c, BS_POWER_SET);
                bs_lexer_expect(&c->lexer, BS_TOKEN_RBRACKET);
            } else {
                bs_chunk_push_op_value(
                    c->bs, c->chunk, BS_OP_CONST, bs_value_object(bs_str_new(c->bs, token.sv)));
            }

            bs_lexer_expect(&c->lexer, BS_TOKEN_SET);
            bs_compile_expr(c, BS_POWER_SET);

            bs_chunk_push_op(c->bs, c->chunk, BS_OP_ISET_CHAIN);
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

            bs_chunk_push_op(c->bs, c->chunk, BS_OP_ISET_CHAIN);

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

        loc = bs_lexer_peek(&c->lexer).loc;
        bs_compile_expr(c, BS_POWER_PRE);
        bs_lexer_expect(&c->lexer, BS_TOKEN_RPAREN);

        bs_chunk_push_op(c->bs, c->chunk, BS_OP_LEN);
        bs_chunk_push_op_loc(c->bs, c->chunk, loc);
        break;

    case BS_TOKEN_IMPORT:
        bs_lexer_expect(&c->lexer, BS_TOKEN_LPAREN);

        loc = bs_lexer_peek(&c->lexer).loc;
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
        bs_compile_lambda(c, BS_LAMBDA_FN, NULL);
        break;

    case BS_TOKEN_THIS: {
        if (!c->class) {
            bs_fmt(
                c->lexer.error,
                Bs_Loc_Fmt "error: cannot use %s outside of %s\n",
                Bs_Loc_Arg(token.loc),
                bs_token_type_name(token.type, c->lexer.extended),
                bs_token_type_name(BS_TOKEN_CLASS, c->lexer.extended));

            bs_lexer_error(&c->lexer);
        }

        const Bs_Sv sv = c->lexer.extended ? Bs_Sv_Static("deez") : Bs_Sv_Static("this");

        size_t index;
        if (bs_lambda_find_local(c->lambda, sv, &index)) {
            bs_chunk_push_op_int(c->bs, c->chunk, BS_OP_LTHIS, index);
        } else if (bs_lambda_find_upvalue(c->bs, c->lambda, sv, &index)) {
            bs_chunk_push_op_int(c->bs, c->chunk, BS_OP_UTHIS, index);
        } else {
            assert(false && "unreachable");
        }
        bs_chunk_push_op_loc(c->bs, c->chunk, token.loc);
    } break;

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
                op_get != BS_OP_LTHIS && op_get != BS_OP_UTHIS && op_get != BS_OP_ISET_CHAIN) {
                bs_compile_error_unexpected(c, &token);
            }

            token = bs_lexer_expect(&c->lexer, BS_TOKEN_IDENT);
            bs_chunk_push_op_value(
                c->bs, c->chunk, BS_OP_IGET_CONST, bs_value_object(bs_str_new(c->bs, token.sv)));
            bs_chunk_push_op_loc(c->bs, c->chunk, loc);
            bs_chunk_push_op_loc(c->bs, c->chunk, token.loc);
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

        case BS_TOKEN_MOD:
            bs_compile_expr(c, lbp);
            bs_chunk_push_op(c->bs, c->chunk, BS_OP_MOD);
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

            const size_t locations_save = c->locations.count;

            size_t arity = 0;
            while (!bs_lexer_read(&c->lexer, BS_TOKEN_RPAREN)) {
                bs_op_locs_push(
                    c->bs, &c->locations, (Bs_Op_Loc){.loc = bs_lexer_peek(&c->lexer).loc});

                bs_compile_expr(c, BS_POWER_SET);
                arity++;

                if (bs_lexer_either(&c->lexer, BS_TOKEN_COMMA, BS_TOKEN_RPAREN).type !=
                    BS_TOKEN_COMMA) {
                    break;
                }
            }

            bs_chunk_push_op_int(c->bs, c->chunk, BS_OP_CALL, arity);
            bs_chunk_push_op_loc(c->bs, c->chunk, loc);

            for (size_t i = locations_save; i < c->locations.count; i++) {
                bs_chunk_push_op_loc(c->bs, c->chunk, c->locations.data[i].loc);
            }
            c->locations.count = locations_save;
        } break;

        case BS_TOKEN_LBRACKET: {
            const Bs_Op op_get = c->chunk->data[c->chunk->last];
            const Bs_Op op_set = bs_op_get_to_set(op_get);
            if (op_set == BS_OP_RET && op_get != BS_OP_CALL && op_get != BS_OP_IMPORT &&
                op_get != BS_OP_LTHIS && op_get != BS_OP_UTHIS && op_get != BS_OP_ISET_CHAIN) {
                bs_compile_error_unexpected(c, &token);
            }

            const Bs_Loc index = bs_lexer_peek(&c->lexer).loc;
            bs_compile_expr(c, BS_POWER_SET);
            bs_lexer_expect(&c->lexer, BS_TOKEN_RBRACKET);

            bs_chunk_push_op(c->bs, c->chunk, BS_OP_IGET);
            bs_chunk_push_op_loc(c->bs, c->chunk, loc);
            bs_chunk_push_op_loc(c->bs, c->chunk, index);
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

            assert(c->chunk->locations.count);

            Bs_Loc locs[2];
            locs[0] = c->chunk->locations.data[--c->chunk->locations.count].loc;
            if (op == BS_OP_ISET || op == BS_OP_ISET_CONST) {
                locs[1] = locs[0];
                locs[0] = c->chunk->locations.data[--c->chunk->locations.count].loc;
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

            bs_chunk_push_op_loc(c->bs, c->chunk, locs[0]);
            if (op == BS_OP_ISET || op == BS_OP_ISET_CONST) {
                bs_chunk_push_op_loc(c->bs, c->chunk, locs[1]);
            }
        } break;

        default:
            assert(false && "unreachable");
        }
    }
}

static void bs_compile_block_init(Bs_Compiler *c) {
    c->lambda->depth++;
}

static size_t bs_compile_block_drop(Bs_Compiler *c, size_t depth) {
    size_t lambda_count = c->lambda->count;
    while (lambda_count && c->lambda->data[lambda_count - 1].depth > depth) {
        if (c->lambda->data[lambda_count - 1].captured) {
            bs_chunk_push_op(c->bs, c->chunk, BS_OP_UCLOSE);
        } else {
            bs_chunk_push_op(c->bs, c->chunk, BS_OP_DROP);
        }

        lambda_count--;
    }

    return lambda_count;
}

static void bs_compile_block_end(Bs_Compiler *c) {
    c->lambda->count = bs_compile_block_drop(c, --c->lambda->depth);
}

static void bs_compile_lambda_init(Bs_Compiler *c, Bs_Lambda *lambda, Bs_Sv name) {
    lambda->fn = bs_fn_new(c->bs);
    lambda->outer = c->lambda;
    c->lambda = lambda;

    if (name.size) {
        lambda->fn->name = bs_str_new(c->bs, name);
    }

    c->chunk = &c->lambda->fn->chunk;

    if (lambda->type == BS_LAMBDA_FN) {
        bs_lambda_push(c->bs, lambda, (Bs_Local){0});
    } else {
        const Bs_Sv sv = c->lexer.extended ? Bs_Sv_Static("deez") : Bs_Sv_Static("this");
        bs_lambda_push(c->bs, lambda, (Bs_Local){.name = sv});
    }
}

static Bs_Fn *bs_compile_lambda_end(Bs_Compiler *c) {
    bs_chunk_push_op(c->bs, c->chunk, BS_OP_NIL);
    bs_chunk_push_op(c->bs, c->chunk, BS_OP_RET);
    Bs_Fn *fn = c->lambda->fn;

    Bs_Lambda *outer = c->lambda->outer;
    c->lambda = outer;

    if (c->lambda) {
        c->chunk = &c->lambda->fn->chunk;
    } else {
        c->chunk = NULL;
    }

    fn->extended = c->lexer.extended;
    return fn;
}

static size_t bs_compile_definition(Bs_Compiler *c, Bs_Token *name, bool public) {
    *name = bs_lexer_expect(&c->lexer, BS_TOKEN_IDENT);

    if (public) {
        bs_values_push(c->bs, &c->chunk->constants, bs_value_object(bs_str_new(c->bs, name->sv)));
        return c->chunk->constants.count - 1;
    }

    bs_da_push(c->bs, c->lambda, ((Bs_Local){.name = name->sv, .depth = c->lambda->depth}));
    return 0;
}

static void bs_compile_stmt(Bs_Compiler *c);

static void bs_compile_lambda(Bs_Compiler *c, Bs_Lambda_Type type, const Bs_Token *name) {
    Bs_Lambda lambda = {.type = type};
    bs_compile_lambda_init(c, &lambda, name ? name->sv : (Bs_Sv){0});
    bs_compile_block_init(c);

    bs_lexer_expect(&c->lexer, BS_TOKEN_LPAREN);
    while (!bs_lexer_read(&c->lexer, BS_TOKEN_RPAREN)) {
        c->lambda->fn->arity++;

        const Bs_Token arg = bs_lexer_expect(&c->lexer, BS_TOKEN_IDENT);
        bs_da_push(c->bs, c->lambda, ((Bs_Local){.name = arg.sv, .depth = c->lambda->depth}));

        if (bs_lexer_either(&c->lexer, BS_TOKEN_COMMA, BS_TOKEN_RPAREN).type != BS_TOKEN_COMMA) {
            break;
        }
    }

    bs_lexer_buffer(&c->lexer, bs_lexer_expect(&c->lexer, BS_TOKEN_LBRACE));
    bs_compile_stmt(c);

    const Bs_Fn *fn = bs_compile_lambda_end(c);
    bs_chunk_push_op_value(c->bs, c->chunk, BS_OP_CLOSURE, bs_value_object(fn));

    for (size_t i = 0; i < lambda.fn->upvalues; i++) {
        bs_chunk_push_op_int(
            c->bs, c->chunk, lambda.uplocals.data[i].local ? 1 : 0, lambda.uplocals.data[i].index);
    }

    bs_lambda_free(c->bs, &lambda);
}

static void bs_compile_class(Bs_Compiler *c, bool public) {
    Bs_Token token;

    size_t const_index = bs_compile_definition(c, &token, true);
    if (!public) {
        bs_da_push(c->bs, c->lambda, ((Bs_Local){.name = token.sv, .depth = c->lambda->depth}));
    }

    bs_chunk_push_op_int(c->bs, c->chunk, BS_OP_CLASS, const_index);
    if (public) {
        bs_chunk_push_op_int(c->bs, c->chunk, BS_OP_GDEF, const_index);
        bs_chunk_push_op_loc(c->bs, c->chunk, token.loc);
    }

    Bs_Class_Compiler class = {0};
    class.outer = c->class;
    c->class = &class;

    bs_compile_identifier(c, &token);
    bs_lexer_expect(&c->lexer, BS_TOKEN_LBRACE);
    while (true) {
        token = bs_lexer_either(&c->lexer, BS_TOKEN_RBRACE, BS_TOKEN_IDENT);
        if (token.type == BS_TOKEN_RBRACE) {
            break;
        }
        bs_lexer_buffer(&c->lexer, token);

        const_index = bs_compile_definition(c, &token, true);
        bs_compile_lambda(c, BS_LAMBDA_METHOD, &token);
        bs_chunk_push_op_int(c->bs, c->chunk, BS_OP_METHOD, const_index);
    }
    bs_chunk_push_op(c->bs, c->chunk, BS_OP_DROP);

    c->class = c->class->outer;
}

static void bs_compile_function(Bs_Compiler *c, bool public) {
    Bs_Token token;

    const size_t const_index = bs_compile_definition(c, &token, public);
    bs_compile_lambda(c, BS_LAMBDA_FN, &token);

    if (public) {
        bs_chunk_push_op_int(c->bs, c->chunk, BS_OP_GDEF, const_index);
        bs_chunk_push_op_loc(c->bs, c->chunk, token.loc);
    }
}

static void bs_compile_variable(Bs_Compiler *c, bool public) {
    Bs_Token token;

    const size_t lambda_index = c->lambda->count;
    const size_t const_index = bs_compile_definition(c, &token, public);

    if (!public) {
        c->lambda->data[lambda_index].name = (Bs_Sv){0};
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
        c->lambda->data[lambda_index].name = token.sv;
    }
}

static Bs_Jumps bs_compile_jumps_save(Bs_Compiler *c, size_t start) {
    const Bs_Jumps save = c->jumps;
    c->jumps.depth = c->lambda->depth;
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

static_assert(BS_COUNT_TOKENS == 59, "Update bs_compile_stmt()");
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

        if (b.type != BS_TOKEN_IN) {
            b = bs_lexer_expect(&c->lexer, BS_TOKEN_IDENT);
            bs_lexer_expect(&c->lexer, BS_TOKEN_IN);
        }

        Bs_Loc locs[3];

        // Container / Start
        locs[0] = bs_lexer_peek(&c->lexer).loc;
        bs_compile_expr(c, BS_POWER_SET);
        bs_lambda_push(c->bs, c->lambda, (Bs_Local){.depth = c->lambda->depth});

        if (b.type == BS_TOKEN_IN) {
            // End
            bs_lexer_expect(&c->lexer, BS_TOKEN_COMMA);

            locs[1] = bs_lexer_peek(&c->lexer).loc;
            bs_compile_expr(c, BS_POWER_SET);
            bs_lambda_push(c->bs, c->lambda, (Bs_Local){.depth = c->lambda->depth});

            // Step
            if (bs_lexer_read(&c->lexer, BS_TOKEN_COMMA)) {
                locs[2] = bs_lexer_peek(&c->lexer).loc;
                bs_compile_expr(c, BS_POWER_SET);
            } else {
                bs_chunk_push_op(c->bs, c->chunk, BS_OP_NIL);
            }
        } else {
            // Iterator
            bs_chunk_push_op(c->bs, c->chunk, BS_OP_NIL);
        }
        bs_lambda_push(c->bs, c->lambda, (Bs_Local){.depth = c->lambda->depth});

        const Bs_Jumps jumps_save = bs_compile_jumps_save(c, c->chunk->count);
        bs_compile_block_init(c);

        // Key
        bs_da_push(c->bs, c->lambda, ((Bs_Local){.name = a.sv, .depth = c->lambda->depth}));

        if (b.type != BS_TOKEN_IN) {
            // Value
            bs_da_push(c->bs, c->lambda, ((Bs_Local){.name = b.sv, .depth = c->lambda->depth}));
        }

        const size_t loop_addr =
            bs_compile_jump_start(c, b.type == BS_TOKEN_IN ? BS_OP_RANGE : BS_OP_ITER);

        bs_chunk_push_op_loc(c->bs, c->chunk, locs[0]);
        if (b.type == BS_TOKEN_IN) {
            bs_chunk_push_op_loc(c->bs, c->chunk, locs[1]);
            bs_chunk_push_op_loc(c->bs, c->chunk, locs[2]);
        }

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
        bs_compile_function(c, false);
        break;

    case BS_TOKEN_PUB:
        if (c->lambda->depth != 1) {
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
        } else if (token.type == BS_TOKEN_CLASS) {
            bs_compile_class(c, true);
        } else {
            bs_fmt(
                c->lexer.error,
                Bs_Loc_Fmt "error: expected %s, %s or %s, got %s\n",
                Bs_Loc_Arg(token.loc),
                bs_token_type_name(BS_TOKEN_FN, c->lexer.extended),
                bs_token_type_name(BS_TOKEN_VAR, c->lexer.extended),
                bs_token_type_name(BS_TOKEN_CLASS, c->lexer.extended),
                bs_token_type_name(token.type, c->lexer.extended));

            bs_lexer_error(&c->lexer);
        }
        break;

    case BS_TOKEN_VAR:
        bs_compile_variable(c, false);
        break;

    case BS_TOKEN_CLASS:
        bs_compile_class(c, false);
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

Bs_Fn *bs_compile_impl(Bs *bs, Bs_Sv path, Bs_Sv input, bool is_main) {
    Bs_Compiler compiler = {
        .bs = bs,
        .is_main = is_main,
    };

    Bs_Lambda lambda = {.type = BS_LAMBDA_FN};
    bs_compile_lambda_init(&compiler, &lambda, path);
    bs_compile_block_init(&compiler);

    {
        // Own the path
        const Bs_Sv path = Bs_Sv(lambda.fn->name->data, lambda.fn->name->size);
        compiler.lexer = bs_lexer_new(path, input, &bs_config(bs)->error);
        compiler.lexer.extended = bs_sv_suffix(path, Bs_Sv_Static(".bsx"));
    }

    if (setjmp(compiler.lexer.unwind)) {
        Bs_Lambda *lambda = compiler.lambda;
        while (lambda) {
            Bs_Lambda *outer = lambda->outer;
            bs_lambda_free(compiler.bs, lambda);
            lambda = outer;
        }

        compiler.lambda = NULL;
        compiler.chunk = NULL;

        bs_jumps_free(compiler.bs, &compiler.jumps);
        bs_op_locs_free(compiler.bs, &compiler.locations);
        return NULL;
    }

    while (!bs_lexer_read(&compiler.lexer, BS_TOKEN_EOF)) {
        bs_compile_stmt(&compiler);
    }

    Bs_Fn *fn = bs_compile_lambda_end(&compiler);
    bs_lambda_free(compiler.bs, &lambda);
    bs_jumps_free(compiler.bs, &compiler.jumps);
    bs_op_locs_free(compiler.bs, &compiler.locations);
    return fn;
}
