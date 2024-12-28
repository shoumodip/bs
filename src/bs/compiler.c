#include "bs/compiler.h"
#include "bs/lexer.h"
#include "bs/op.h"

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
    BS_POWER_IN,
    BS_POWER_DOT,
} Bs_Power;

static_assert(BS_COUNT_TOKENS == 77, "Update bs_token_type_power()");
static Bs_Power bs_token_type_power(Bs_Token_Type type) {
    switch (type) {
    case BS_TOKEN_DOT:
    case BS_TOKEN_LPAREN:
    case BS_TOKEN_LBRACKET:
        return BS_POWER_DOT;

    case BS_TOKEN_IN:
    case BS_TOKEN_IS:
    case BS_TOKEN_LNOT:
        return BS_POWER_IN;

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
    case BS_TOKEN_ADD_SET:
    case BS_TOKEN_SUB_SET:
    case BS_TOKEN_MUL_SET:
    case BS_TOKEN_DIV_SET:
    case BS_TOKEN_MOD_SET:
    case BS_TOKEN_BOR_SET:
    case BS_TOKEN_BAND_SET:
    case BS_TOKEN_BXOR_SET:
    case BS_TOKEN_SHL_SET:
    case BS_TOKEN_SHR_SET:
    case BS_TOKEN_JOIN_SET:
        return BS_POWER_SET;

    default:
        return BS_POWER_NIL;
    }
}

static_assert(BS_COUNT_TOKENS == 77, "Update bs_token_type_can_start()");
static bool bs_token_type_can_start(Bs_Token_Type type) {
    switch (type) {
    case BS_TOKEN_SPREAD:
    case BS_TOKEN_NIL:
    case BS_TOKEN_STR:
    case BS_TOKEN_ISTR:
    case BS_TOKEN_RSTR:
    case BS_TOKEN_NUM:
    case BS_TOKEN_TRUE:
    case BS_TOKEN_FALSE:
    case BS_TOKEN_IDENT:
    case BS_TOKEN_LPAREN:
    case BS_TOKEN_LBRACE:
    case BS_TOKEN_LBRACKET:
    case BS_TOKEN_SUB:
    case BS_TOKEN_BNOT:
    case BS_TOKEN_LNOT:
    case BS_TOKEN_JOIN:
    case BS_TOKEN_LEN:
    case BS_TOKEN_PANIC:
    case BS_TOKEN_ASSERT:
    case BS_TOKEN_DELETE:
    case BS_TOKEN_IMPORT:
    case BS_TOKEN_TYPEOF:
    case BS_TOKEN_IF:
    case BS_TOKEN_FN:
    case BS_TOKEN_THIS:
    case BS_TOKEN_SUPER:
    case BS_TOKEN_IS_MAIN_MODULE:
        return true;

    default:
        return false;
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
    bool can_fail;
    bool has_super;
};

typedef enum {
    BS_LAMBDA_FN,
    BS_LAMBDA_INIT,
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

    Bs_Jumps jumps;
    Bs_Jumps matches;

    bool is_repl;
    bool is_meta;
};

#define bs_lambda_push bs_da_push

static void bs_lambda_free(Bs *bs, Bs_Lambda *l) {
    if (l) {
        bs_uplocals_free(bs, &l->uplocals);
        bs_jumps_free(bs, &l->jumps);
        bs_jumps_free(bs, &l->matches);
        bs_da_free(bs, l);
        free(l);
    }
}

static Bs_Lambda *bs_lambda_new(Bs_Lambda_Type type, bool is_repl, bool is_meta) {
    // The lambdas need to stored on the heap since the error handling mechanism uses longjumps
    // which changes the stack pointer
    Bs_Lambda *p = malloc(sizeof(Bs_Lambda));
    assert(p);
    *p = (Bs_Lambda){.type = type, .is_repl = is_repl, .is_meta = is_meta};
    return p;
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

    Bs_Op_Locs locations;

    size_t module;

    bool is_main;
    bool last_stmt_was_expr;

    // Fixes this bug:
    //
    // (if x then a.y else a.z)(...) => (if x then a.y else a.z(...))
    bool last_expr_was_if;
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
    bs_lexer_error(&c->lexer, token->loc, "unexpected %s", bs_token_type_name(token->type));
}

static void bs_compile_consume_eol(Bs_Compiler *c) {
    while (bs_lexer_read(&c->lexer, BS_TOKEN_EOL)) {}
}

static void bs_compile_lambda(Bs_Compiler *c, Bs_Lambda_Type type, const Bs_Token *name);

static void bs_compile_string(Bs_Compiler *c, Bs_Sv sv) {
    Bs_Buffer *b = &bs_config(c->bs)->buffer;
    const size_t start = b->count;

    for (size_t i = 0; i < sv.size; i++) {
        char ch = sv.data[i];
        if (ch == '\\') {
            switch (sv.data[++i]) {
            case 'e':
                ch = '\033';
                break;

            case 'n':
                ch = '\n';
                break;

            case 'r':
                ch = '\r';
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

            case '\'':
                ch = '\'';
                break;

            case '\\':
                ch = '\\';
                break;

            case '{':
                ch = '{';
                break;

            default:
                assert(false && "unreachable");
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

static void bs_compile_receiver(Bs_Compiler *c, Bs_Sv sv, Bs_Loc loc) {
    size_t index;
    if (bs_lambda_find_local(c->lambda, sv, &index)) {
        bs_chunk_push_op_int(c->bs, c->chunk, BS_OP_LRECEIVER, index);
    } else if (bs_lambda_find_upvalue(c->bs, c->lambda, sv, &index)) {
        bs_chunk_push_op_int(c->bs, c->chunk, BS_OP_URECEIVER, index);
    } else {
        assert(false && "unreachable");
    }
    bs_chunk_push_op_loc(c->bs, c->chunk, loc);
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

static void bs_compile_expr(Bs_Compiler *c, Bs_Power mbp);

static void bs_compile_assignment(Bs_Compiler *c, const Bs_Token *token, Bs_Op arith_op) {
    const Bs_Op assign_op = bs_op_get_to_set(c->chunk->data[c->chunk->last]);
    if (assign_op == BS_OP_RET) {
        bs_compile_error_unexpected(c, token);
    }

    assert(c->chunk->locations.count);
    const size_t locations_count_save = c->chunk->locations.count;

    Bs_Loc locs[2];
    locs[0] = c->chunk->locations.data[--c->chunk->locations.count].loc;
    if (assign_op == BS_OP_ISET || assign_op == BS_OP_ISET_CONST) {
        assert(c->chunk->locations.count);
        locs[1] = locs[0];
        locs[0] = c->chunk->locations.data[--c->chunk->locations.count].loc;
    }

    size_t index;
    if (assign_op != BS_OP_ISET) {
        index = *(size_t *)&c->chunk->data[c->chunk->last + 1];
    }

    if (arith_op == BS_OP_RET) {
        c->chunk->count = c->chunk->last;
    } else {
        c->chunk->locations.count = locations_count_save;

        if (assign_op == BS_OP_ISET) {
            c->chunk->count = c->chunk->last;

            bs_chunk_push_op(c->bs, c->chunk, BS_OP_DUP);
            bs_da_push(c->bs, c->chunk, 1);

            bs_chunk_push_op(c->bs, c->chunk, BS_OP_DUP);
            bs_da_push(c->bs, c->chunk, 1);

            bs_chunk_push_op(c->bs, c->chunk, BS_OP_IGET);
            bs_chunk_push_op_loc(c->bs, c->chunk, locs[0]);
            bs_chunk_push_op_loc(c->bs, c->chunk, locs[1]);
        } else if (assign_op == BS_OP_ISET_CONST) {
            c->chunk->count = c->chunk->last;

            bs_chunk_push_op(c->bs, c->chunk, BS_OP_DUP);
            bs_da_push(c->bs, c->chunk, 0);

            bs_chunk_push_op_int(c->bs, c->chunk, BS_OP_IGET_CONST, index);
            bs_chunk_push_op_loc(c->bs, c->chunk, locs[0]);
            bs_chunk_push_op_loc(c->bs, c->chunk, locs[1]);
        }
    }

    bs_compile_expr(c, BS_POWER_SET);

    if (arith_op != BS_OP_RET) {
        bs_chunk_push_op(c->bs, c->chunk, arith_op);
        bs_chunk_push_op_loc(c->bs, c->chunk, token->loc);
    }

    if (assign_op == BS_OP_ISET) {
        bs_chunk_push_op(c->bs, c->chunk, assign_op);
    } else {
        bs_chunk_push_op_int(c->bs, c->chunk, assign_op, index);
    }

    bs_chunk_push_op_loc(c->bs, c->chunk, locs[0]);
    if (assign_op == BS_OP_ISET || assign_op == BS_OP_ISET_CONST) {
        bs_chunk_push_op_loc(c->bs, c->chunk, locs[1]);
    }
}

static_assert(BS_COUNT_TOKENS == 77, "Update bs_compile_expr()");
static void bs_compile_expr(Bs_Compiler *c, Bs_Power mbp) {
    Bs_Token token = bs_lexer_next(&c->lexer);
    Bs_Loc loc = token.loc;

    c->last_expr_was_if = false;
    switch (token.type) {
    case BS_TOKEN_NIL:
        bs_chunk_push_op(c->bs, c->chunk, BS_OP_NIL);
        break;

    case BS_TOKEN_STR:
        bs_compile_string(c, token.sv);
        break;

    case BS_TOKEN_ISTR: {
        bs_compile_string(c, token.sv);

        const char end = token.sv.data[-1];
        while (token.type == BS_TOKEN_ISTR) {
            bs_compile_expr(c, BS_POWER_SET);
            bs_lexer_expect(&c->lexer, BS_TOKEN_RBRACE);

            bs_chunk_push_op(c->bs, c->chunk, BS_OP_JOIN);

            token = bs_lexer_str(&c->lexer, c->lexer.loc, end);

            if (token.sv.size) {
                bs_compile_string(c, token.sv);
                bs_chunk_push_op(c->bs, c->chunk, BS_OP_JOIN);
            }
        }
    } break;

    case BS_TOKEN_RSTR: {
        Bs_Buffer *b = &bs_config(c->bs)->buffer;
        const size_t start = b->count;

        Bs_Sv sv = token.sv;

        size_t count = 0;
        while (count < sv.size && sv.data[count] == '{') {
            count++;
        }

        bs_sv_drop(&sv, count);
        sv.size -= count;

        assert(sv.size);
        bs_sv_drop(&sv, 1); // Drop the beginning newline

        size_t indent = 0;
        bool indent_set = false;
        bool first = true;
        while (sv.size) {
            const Bs_Sv line = bs_sv_split(&sv, '\n');
            if (!indent_set) {
                const Bs_Sv trimmed = bs_sv_trim(line, ' ');
                if (trimmed.size) {
                    indent = trimmed.data - line.data;
                    indent_set = true;
                }
            }

            if (first) {
                first = false;
            } else {
                bs_da_push(c->bs, b, '\n');
            }

            if (indent_set && line.size > indent) {
                bs_da_push_many(c->bs, b, line.data + indent, line.size - indent);
            }
        }

        bs_chunk_push_op_value(
            c->bs,
            c->chunk,
            BS_OP_CONST,
            bs_value_object(bs_str_new(c->bs, bs_buffer_reset(b, start))));
    } break;

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
            const Bs_Token_Type expected[] = {
                BS_TOKEN_IDENT,
                BS_TOKEN_LBRACKET,
                BS_TOKEN_SPREAD,
            };
            token = bs_lexer_one_of(&c->lexer, expected, bs_c_array_size(expected));
            loc = token.loc;

            if (token.type == BS_TOKEN_IDENT) {
                bs_chunk_push_op_value(
                    c->bs, c->chunk, BS_OP_CONST, bs_value_object(bs_str_new(c->bs, token.sv)));
            } else if (token.type == BS_TOKEN_LBRACKET) {
                loc = bs_lexer_peek(&c->lexer).loc;
                bs_compile_expr(c, BS_POWER_SET);
                bs_lexer_expect(&c->lexer, BS_TOKEN_RBRACKET);
            } else if (token.type == BS_TOKEN_SPREAD) {
                bs_lexer_unbuffer(&c->lexer);
                bs_compile_expr(c, BS_POWER_IN);
                bs_chunk_push_op(c->bs, c->chunk, BS_OP_APPEND);
                bs_da_push(c->bs, c->chunk, 1);
                bs_chunk_push_op_loc(c->bs, c->chunk, token.loc);
            } else {
                assert(false && "unreachable");
            }

            if (token.type != BS_TOKEN_SPREAD) {
                bs_lexer_expect(&c->lexer, BS_TOKEN_SET);
                bs_compile_expr(c, BS_POWER_SET);

                bs_chunk_push_op(c->bs, c->chunk, BS_OP_ISET_CHAIN);
                bs_chunk_push_op_loc(c->bs, c->chunk, loc);
            }

            if (bs_lexer_either(&c->lexer, BS_TOKEN_COMMA, BS_TOKEN_RBRACE).type !=
                BS_TOKEN_COMMA) {
                break;
            }
        }
    } break;

    case BS_TOKEN_LBRACKET: {
        bs_chunk_push_op(c->bs, c->chunk, BS_OP_ARRAY);

        while (!bs_lexer_read(&c->lexer, BS_TOKEN_RBRACKET)) {
            token = bs_lexer_peek(&c->lexer);
            if (token.type == BS_TOKEN_SPREAD) {
                bs_lexer_unbuffer(&c->lexer);
                bs_compile_expr(c, BS_POWER_IN);
                bs_chunk_push_op(c->bs, c->chunk, BS_OP_APPEND);
                bs_da_push(c->bs, c->chunk, 1);
                bs_chunk_push_op_loc(c->bs, c->chunk, token.loc);
            } else {
                bs_compile_expr(c, BS_POWER_SET);
                bs_chunk_push_op(c->bs, c->chunk, BS_OP_APPEND);
                bs_da_push(c->bs, c->chunk, 0);
            }

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

    case BS_TOKEN_JOIN:
        bs_compile_expr(c, BS_POWER_PRE);
        bs_chunk_push_op(c->bs, c->chunk, BS_OP_TOSTR);
        break;

    case BS_TOKEN_LEN:
        bs_lexer_expect(&c->lexer, BS_TOKEN_LPAREN);

        loc = bs_lexer_peek(&c->lexer).loc;
        bs_compile_expr(c, BS_POWER_PRE);
        bs_lexer_expect(&c->lexer, BS_TOKEN_RPAREN);

        bs_chunk_push_op(c->bs, c->chunk, BS_OP_LEN);
        bs_chunk_push_op_loc(c->bs, c->chunk, loc);
        break;

    case BS_TOKEN_PANIC:
        bs_lexer_expect(&c->lexer, BS_TOKEN_LPAREN);

        if (bs_lexer_read(&c->lexer, BS_TOKEN_RPAREN)) {
            bs_chunk_push_op_value(
                c->bs,
                c->chunk,
                BS_OP_CONST,
                bs_value_object(bs_str_new(c->bs, Bs_Sv_Static("panic"))));
        } else {
            bs_compile_expr(c, BS_POWER_SET);
            bs_lexer_expect(&c->lexer, BS_TOKEN_RPAREN);
        }

        bs_chunk_push_op(c->bs, c->chunk, BS_OP_PANIC);
        bs_chunk_push_op_loc(c->bs, c->chunk, loc);
        break;

    case BS_TOKEN_ASSERT:
        bs_lexer_expect(&c->lexer, BS_TOKEN_LPAREN);
        bs_compile_expr(c, BS_POWER_SET);

        if (bs_lexer_either(&c->lexer, BS_TOKEN_COMMA, BS_TOKEN_RPAREN).type == BS_TOKEN_COMMA) {
            bs_compile_expr(c, BS_POWER_SET);
            bs_lexer_expect(&c->lexer, BS_TOKEN_RPAREN);
        } else {
            bs_chunk_push_op_value(
                c->bs,
                c->chunk,
                BS_OP_CONST,
                bs_value_object(bs_str_new(c->bs, Bs_Sv_Static("assertion failed"))));
        }

        bs_chunk_push_op(c->bs, c->chunk, BS_OP_ASSERT);
        bs_chunk_push_op_loc(c->bs, c->chunk, loc);
        break;

    case BS_TOKEN_DELETE: {
        bs_lexer_expect(&c->lexer, BS_TOKEN_LPAREN);

        loc = bs_lexer_peek(&c->lexer).loc;
        bs_compile_expr(c, BS_POWER_SET);
        bs_lexer_expect(&c->lexer, BS_TOKEN_RPAREN);

        uint8_t *op = &c->chunk->data[c->chunk->last];
        if (*op == BS_OP_IGET) {
            *op = BS_OP_DELETE;
        } else if (*op == BS_OP_IGET_CONST) {
            *op = BS_OP_DELETE_CONST;
        } else if (*op == BS_OP_SUPER_GET) {
            bs_lexer_error(&c->lexer, loc, "cannot use 'delete' on super");
        } else {
            bs_lexer_error_full(
                &c->lexer,
                loc,
                Bs_Sv_Static("Index expression can be any of the following:"),
                Bs_Sv_Static("xs.foo;    // Constant index\n"
                             "xs[\"bar\"]; // Expression based index"),
                "expected index expression");
        }
    } break;

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

        c->last_expr_was_if = true;
    } break;

    case BS_TOKEN_FN:
        bs_compile_lambda(c, BS_LAMBDA_FN, NULL);
        break;

    case BS_TOKEN_THIS:
        if (!c->class) {
            bs_lexer_error(
                &c->lexer,
                token.loc,
                "cannot use %s outside of %s",
                bs_token_type_name(token.type),
                bs_token_type_name(BS_TOKEN_CLASS));
        }

        bs_compile_receiver(c, Bs_Sv_Static("this"), token.loc);
        break;

    case BS_TOKEN_SUPER: {
        if (!c->class) {
            bs_lexer_error(
                &c->lexer,
                token.loc,
                "cannot use %s outside of %s",
                bs_token_type_name(token.type),
                bs_token_type_name(BS_TOKEN_CLASS));
        }

        if (!c->class->has_super) {
            bs_lexer_error(
                &c->lexer,
                token.loc,
                "cannot use %s outside of an inheriting class",
                bs_token_type_name(token.type));
        }

        bs_lexer_expect(&c->lexer, BS_TOKEN_DOT);
        const Bs_Token method = bs_lexer_expect(&c->lexer, BS_TOKEN_IDENT);

        bs_compile_receiver(c, Bs_Sv_Static("this"), token.loc);
        bs_compile_receiver(c, Bs_Sv_Static("super"), token.loc);

        if (bs_sv_eq(method.sv, Bs_Sv_Static("init"))) {
            bs_chunk_push_op_value(c->bs, c->chunk, BS_OP_SUPER_GET, bs_value_nil);
        } else {
            bs_chunk_push_op_value(
                c->bs, c->chunk, BS_OP_SUPER_GET, bs_value_object(bs_str_new(c->bs, method.sv)));
        }
        bs_chunk_push_op_loc(c->bs, c->chunk, method.loc);
    } break;

    case BS_TOKEN_IS_MAIN_MODULE:
        bs_chunk_push_op(c->bs, c->chunk, c->is_main ? BS_OP_TRUE : BS_OP_FALSE);
        break;

    default:
        bs_compile_error_unexpected(c, &token);
        return;
    }

    while (true) {
        if (mbp == BS_POWER_NIL) {
            // Top level expression, perform ASI
            if (!bs_lexer_peek_row(&c->lexer, &token) && token.type != BS_TOKEN_DOT) {
                break;
            }
        } else {
            // No ASI
            token = bs_lexer_peek(&c->lexer);
        }

        const Bs_Power lbp = bs_token_type_power(token.type);
        if (lbp <= mbp) {
            break;
        }
        bs_lexer_unbuffer(&c->lexer);

        loc = token.loc;
        switch (token.type) {
        case BS_TOKEN_DOT: {
            const Bs_Op op = c->chunk->data[c->chunk->last];
            if (op == BS_OP_GSET || op == BS_OP_LSET || op == BS_OP_USET || op == BS_OP_ISET ||
                op == BS_OP_ISET_CONST || op == BS_OP_CLOSURE) {
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

            if (op_set == BS_OP_RET && op_get != BS_OP_CALL && op_get != BS_OP_INVOKE &&
                op_get != BS_OP_SUPER_GET && op_get != BS_OP_IMPORT && op_get != BS_OP_CLOSURE) {
                bs_compile_error_unexpected(c, &token);
            }

            Bs_Loc locs[2];
            size_t call_const_index = 0;
            size_t super_const_index = 0;

            const bool was_if = c->last_expr_was_if;
            if (!was_if) {
                if (op_get == BS_OP_IGET_CONST) {
                    call_const_index = *(size_t *)&c->chunk->data[c->chunk->last + 1];
                    c->chunk->count = c->chunk->last;

                    locs[1] = c->chunk->locations.data[--c->chunk->locations.count].loc;
                    locs[0] = c->chunk->locations.data[--c->chunk->locations.count].loc;
                } else if (op_get == BS_OP_SUPER_GET) {
                    call_const_index = *(size_t *)&c->chunk->data[c->chunk->last + 1];

                    assert(c->chunk->last > 1 + sizeof(size_t));
                    c->chunk->count = c->chunk->last - 1 - sizeof(size_t);

                    assert(c->chunk->data[c->chunk->count] == BS_OP_URECEIVER);
                    super_const_index = *(size_t *)&c->chunk->data[c->chunk->count + 1];

                    locs[0] = c->chunk->locations.data[--c->chunk->locations.count].loc;
                }
            }

            const size_t locations_save = c->locations.count;

            bs_chunk_push_op(c->bs, c->chunk, BS_OP_CALL_INIT);
            while (!bs_lexer_read(&c->lexer, BS_TOKEN_RPAREN)) {
                token = bs_lexer_peek(&c->lexer);
                bs_op_locs_push(c->bs, &c->locations, (Bs_Op_Loc){.loc = token.loc});

                if (token.type == BS_TOKEN_SPREAD) {
                    bs_lexer_unbuffer(&c->lexer);
                    bs_compile_expr(c, BS_POWER_IN);
                    bs_chunk_push_op(c->bs, c->chunk, BS_OP_SPREAD);
                    bs_chunk_push_op_loc(c->bs, c->chunk, token.loc);
                } else {
                    bs_compile_expr(c, BS_POWER_SET);
                }

                if (bs_lexer_either(&c->lexer, BS_TOKEN_COMMA, BS_TOKEN_RPAREN).type !=
                    BS_TOKEN_COMMA) {
                    break;
                }
            }

            if (!was_if && op_get == BS_OP_IGET_CONST) {
                bs_chunk_push_op_int(c->bs, c->chunk, BS_OP_INVOKE, call_const_index);
                bs_chunk_push_op_loc(c->bs, c->chunk, locs[0]);
                bs_chunk_push_op_loc(c->bs, c->chunk, locs[1]);
            } else if (!was_if && op_get == BS_OP_SUPER_GET) {
                bs_chunk_push_op_int(c->bs, c->chunk, BS_OP_URECEIVER, super_const_index);
                bs_chunk_push_op_int(c->bs, c->chunk, BS_OP_SUPER_INVOKE, call_const_index);
                bs_chunk_push_op_loc(c->bs, c->chunk, locs[0]);
            } else {
                bs_chunk_push_op(c->bs, c->chunk, BS_OP_CALL);
            }
            bs_chunk_push_op_loc(c->bs, c->chunk, loc);

            for (size_t i = locations_save; i < c->locations.count; i++) {
                bs_chunk_push_op_loc(c->bs, c->chunk, c->locations.data[i].loc);
            }
            c->locations.count = locations_save;
        } break;

        case BS_TOKEN_LBRACKET: {
            const Bs_Op op = c->chunk->data[c->chunk->last];
            if (op == BS_OP_GSET || op == BS_OP_LSET || op == BS_OP_USET || op == BS_OP_ISET ||
                op == BS_OP_ISET_CONST || op == BS_OP_CLOSURE) {
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

        case BS_TOKEN_SET:
            bs_compile_assignment(c, &token, BS_OP_RET);
            break;

        case BS_TOKEN_ADD_SET:
            bs_compile_assignment(c, &token, BS_OP_ADD);
            break;

        case BS_TOKEN_SUB_SET:
            bs_compile_assignment(c, &token, BS_OP_SUB);
            break;

        case BS_TOKEN_MUL_SET:
            bs_compile_assignment(c, &token, BS_OP_MUL);
            break;

        case BS_TOKEN_DIV_SET:
            bs_compile_assignment(c, &token, BS_OP_DIV);
            break;

        case BS_TOKEN_MOD_SET:
            bs_compile_assignment(c, &token, BS_OP_MOD);
            break;

        case BS_TOKEN_BOR_SET:
            bs_compile_assignment(c, &token, BS_OP_BOR);
            break;

        case BS_TOKEN_BAND_SET:
            bs_compile_assignment(c, &token, BS_OP_BAND);
            break;

        case BS_TOKEN_BXOR_SET:
            bs_compile_assignment(c, &token, BS_OP_BXOR);
            break;

        case BS_TOKEN_SHL_SET:
            bs_compile_assignment(c, &token, BS_OP_SHL);
            break;

        case BS_TOKEN_SHR_SET:
            bs_compile_assignment(c, &token, BS_OP_SHR);
            break;

        case BS_TOKEN_JOIN_SET:
            bs_compile_assignment(c, &token, BS_OP_JOIN);
            break;

        case BS_TOKEN_IN:
            bs_compile_expr(c, lbp);
            bs_chunk_push_op(c->bs, c->chunk, BS_OP_IN);
            bs_chunk_push_op_loc(c->bs, c->chunk, loc);
            break;

        case BS_TOKEN_IS:
            loc = bs_lexer_peek(&c->lexer).loc;
            bs_compile_expr(c, lbp);
            bs_chunk_push_op(c->bs, c->chunk, BS_OP_IS);
            bs_chunk_push_op_loc(c->bs, c->chunk, loc);
            break;

        case BS_TOKEN_LNOT:
            token = bs_lexer_either(&c->lexer, BS_TOKEN_IN, BS_TOKEN_IS);
            if (token.type == BS_TOKEN_IN) {
                bs_compile_expr(c, lbp);
                bs_chunk_push_op(c->bs, c->chunk, BS_OP_IN);
                bs_chunk_push_op_loc(c->bs, c->chunk, loc);
            } else {
                loc = bs_lexer_peek(&c->lexer).loc;
                bs_compile_expr(c, lbp);
                bs_chunk_push_op(c->bs, c->chunk, BS_OP_IS);
                bs_chunk_push_op_loc(c->bs, c->chunk, loc);
            }
            bs_chunk_push_op(c->bs, c->chunk, BS_OP_LNOT);
            break;

        default:
            assert(false && "unreachable");
        }

        c->last_expr_was_if = false;
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
    c->lambda->fn->compiled_in_module = c->module;

    if (lambda->type == BS_LAMBDA_FN) {
        bs_lambda_push(c->bs, lambda, (Bs_Local){0});
    } else {
        bs_lambda_push(c->bs, lambda, (Bs_Local){.name = Bs_Sv_Static("this")});
    }
}

static Bs_Fn *bs_compile_lambda_end(Bs_Compiler *c) {
    if (c->lambda->type == BS_LAMBDA_INIT) {
        bs_chunk_push_op_int(c->bs, c->chunk, BS_OP_LRECEIVER, 0);
    } else {
        bs_chunk_push_op(c->bs, c->chunk, BS_OP_NIL);
    }
    bs_chunk_push_op(c->bs, c->chunk, BS_OP_RET);
    Bs_Fn *fn = c->lambda->fn;

    Bs_Lambda *outer = c->lambda->outer;
    c->lambda = outer;

    if (c->lambda) {
        c->chunk = &c->lambda->fn->chunk;
    } else {
        c->chunk = NULL;
    }

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
    Bs_Lambda *lambda = bs_lambda_new(type, false, false);
    bs_compile_lambda_init(c, lambda, name ? name->sv : (Bs_Sv){0});
    bs_compile_block_init(c);

    bs_lexer_expect(&c->lexer, BS_TOKEN_LPAREN);

    bool variadic = false;
    while (!bs_lexer_read(&c->lexer, BS_TOKEN_RPAREN)) {
        Bs_Token arg = bs_lexer_either(&c->lexer, BS_TOKEN_IDENT, BS_TOKEN_SPREAD);
        c->lambda->fn->arity++;

        if (arg.type == BS_TOKEN_SPREAD) {
            variadic = true;
            arg = bs_lexer_expect(&c->lexer, BS_TOKEN_IDENT);
        }
        bs_da_push(c->bs, c->lambda, ((Bs_Local){.name = arg.sv, .depth = c->lambda->depth}));

        if (variadic) {
            bs_lexer_expect(&c->lexer, BS_TOKEN_RPAREN);
            break;
        }

        if (bs_lexer_either(&c->lexer, BS_TOKEN_COMMA, BS_TOKEN_RPAREN).type != BS_TOKEN_COMMA) {
            break;
        }
    }

    bs_lexer_buffer(&c->lexer, bs_lexer_either(&c->lexer, BS_TOKEN_LBRACE, BS_TOKEN_ARROW));
    if (c->lexer.buffer.type == BS_TOKEN_LBRACE) {
        bs_compile_stmt(c);
    } else {
        bs_lexer_unbuffer(&c->lexer);
        bs_compile_expr(c, BS_POWER_SET);
        bs_chunk_push_op(c->bs, c->chunk, BS_OP_RET);
    }

    Bs_Fn *fn = bs_compile_lambda_end(c);
    fn->variadic = variadic;

    bs_chunk_push_op_value(c->bs, c->chunk, BS_OP_CLOSURE, bs_value_object(fn));

    for (size_t i = 0; i < lambda->fn->upvalues; i++) {
        bs_chunk_push_op_int(
            c->bs,
            c->chunk,
            lambda->uplocals.data[i].local ? 1 : 0,
            lambda->uplocals.data[i].index);
    }

    bs_lambda_free(c->bs, lambda);
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

    if (bs_lexer_read(&c->lexer, BS_TOKEN_LT)) {
        const Bs_Token super = bs_lexer_expect(&c->lexer, BS_TOKEN_IDENT);
        if (bs_sv_eq(super.sv, token.sv)) {
            bs_lexer_error(&c->lexer, super.loc, "a class cannot inherit from itself");
        }
        bs_compile_identifier(c, &super);

        bs_compile_block_init(c);
        bs_lambda_push(
            c->bs,
            c->lambda,
            ((Bs_Local){.name = Bs_Sv_Static("super"), .depth = c->lambda->depth}));

        bs_compile_identifier(c, &token);
        bs_chunk_push_op(c->bs, c->chunk, BS_OP_INHERIT);
        bs_chunk_push_op_loc(c->bs, c->chunk, super.loc);

        c->class->has_super = true;
    }

    bs_compile_identifier(c, &token);
    bs_lexer_expect(&c->lexer, BS_TOKEN_LBRACE);
    while (true) {
        Bs_Token method = bs_lexer_either(&c->lexer, BS_TOKEN_RBRACE, BS_TOKEN_IDENT);
        if (method.type == BS_TOKEN_RBRACE) {
            break;
        }

        if (bs_sv_eq(method.sv, Bs_Sv_Static("init"))) {
            bs_compile_lambda(c, BS_LAMBDA_INIT, &token);
            bs_chunk_push_op(c->bs, c->chunk, BS_OP_INIT_METHOD);
            bs_da_push(c->bs, c->chunk, c->class->can_fail);
        } else {
            bs_lexer_buffer(&c->lexer, method);
            const_index = bs_compile_definition(c, &method, true);

            bs_compile_lambda(c, BS_LAMBDA_METHOD, &method);
            bs_chunk_push_op_int(c->bs, c->chunk, BS_OP_METHOD, const_index);
        }
    }
    bs_chunk_push_op(c->bs, c->chunk, BS_OP_DROP);

    if (c->class->has_super) {
        bs_compile_block_end(c);
    }

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

    if (public) {
        bs_chunk_push_op_int(c->bs, c->chunk, BS_OP_GDEF, const_index);
        bs_chunk_push_op_loc(c->bs, c->chunk, token.loc);
    } else {
        c->lambda->data[lambda_index].name = token.sv;
    }
}

static Bs_Jumps bs_compile_jumps_save(Bs_Compiler *c, size_t start) {
    const Bs_Jumps save = c->lambda->jumps;
    c->lambda->jumps.depth = c->lambda->depth;
    c->lambda->jumps.start = start;
    return save;
}

static void bs_compile_jumps_reset(Bs_Compiler *c, Bs_Jumps save) {
    for (size_t i = save.count; i < c->lambda->jumps.count; i++) {
        bs_compile_jump_patch(c, c->lambda->jumps.data[i]);
    }

    c->lambda->jumps.count = save.count;
    c->lambda->jumps.depth = save.depth;
    c->lambda->jumps.start = save.start;
}

static_assert(BS_COUNT_TOKENS == 77, "Update bs_compile_stmt()");
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
            const Bs_Token_Type expected[] = {
                BS_TOKEN_LBRACE,
                BS_TOKEN_IF,
                BS_TOKEN_MATCH,
            };

            bs_lexer_buffer(
                &c->lexer, bs_lexer_one_of(&c->lexer, expected, bs_c_array_size(expected)));
            bs_compile_stmt(c);
        }
        bs_compile_jump_patch(c, else_addr);
    } break;

    case BS_TOKEN_MATCH: {
        bs_compile_expr(c, BS_POWER_SET);
        bs_lexer_expect(&c->lexer, BS_TOKEN_LBRACE);

        const size_t matches_count_save = c->lambda->matches.count;

        while (!bs_lexer_read(&c->lexer, BS_TOKEN_RBRACE)) {
            const size_t cases_count_save = c->lambda->matches.count;

            do {
                token = bs_lexer_peek(&c->lexer);
                if (token.type == BS_TOKEN_IF) {
                    bs_lexer_unbuffer(&c->lexer);
                    bs_compile_expr(c, BS_POWER_SET);
                    bs_jumps_push(c->bs, &c->lambda->matches, c->chunk->count);
                    bs_chunk_push_op_int(c->bs, c->chunk, BS_OP_MATCH_IF, 0);
                } else {
                    bs_compile_expr(c, BS_POWER_SET);
                    bs_jumps_push(c->bs, &c->lambda->matches, c->chunk->count);
                    bs_chunk_push_op_int(c->bs, c->chunk, BS_OP_MATCH, 0);
                }

                token = bs_lexer_either(&c->lexer, BS_TOKEN_COMMA, BS_TOKEN_ARROW);
            } while (token.type == BS_TOKEN_COMMA);

            const size_t skip_addr = bs_compile_jump_start(c, BS_OP_JUMP);

            for (size_t i = cases_count_save; i < c->lambda->matches.count; i++) {
                bs_compile_jump_patch(c, c->lambda->matches.data[i]);
            }
            c->lambda->matches.count = cases_count_save;

            bs_chunk_push_op(c->bs, c->chunk, BS_OP_DROP);

            token = bs_lexer_peek(&c->lexer);
            if (token.type == BS_TOKEN_VAR || token.type == BS_TOKEN_FN ||
                token.type == BS_TOKEN_CLASS) {
                bs_lexer_error(
                    &c->lexer,
                    token.loc,
                    "cannot use %s here without wrapping in {}",
                    bs_token_type_name(token.type));
            }
            bs_compile_stmt(c);

            bs_jumps_push(c->bs, &c->lambda->matches, c->chunk->count);
            bs_chunk_push_op_int(c->bs, c->chunk, BS_OP_JUMP, 0);

            bs_compile_jump_patch(c, skip_addr);
        }
        bs_chunk_push_op(c->bs, c->chunk, BS_OP_DROP);

        if (bs_lexer_read(&c->lexer, BS_TOKEN_ELSE)) {
            const Bs_Token_Type expected[] = {
                BS_TOKEN_LBRACE,
                BS_TOKEN_IF,
                BS_TOKEN_MATCH,
            };

            bs_lexer_buffer(
                &c->lexer, bs_lexer_one_of(&c->lexer, expected, bs_c_array_size(expected)));
            bs_compile_stmt(c);
        }

        for (size_t i = matches_count_save; i < c->lambda->matches.count; i++) {
            bs_compile_jump_patch(c, c->lambda->matches.data[i]);
        }
        c->lambda->matches.count = matches_count_save;
    } break;

    case BS_TOKEN_FOR: {
        bs_compile_block_init(c);

        const Bs_Token a = bs_lexer_expect(&c->lexer, BS_TOKEN_IDENT);
        Bs_Token b = bs_lexer_either(&c->lexer, BS_TOKEN_COMMA, BS_TOKEN_IN);

        if (b.type != BS_TOKEN_IN) {
            b = bs_lexer_expect(&c->lexer, BS_TOKEN_IDENT);
            bs_lexer_expect(&c->lexer, BS_TOKEN_IN);
        }

        Bs_Loc locs[3] = {0};

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
            if (locs[2].row) {
                bs_chunk_push_op_loc(c->bs, c->chunk, locs[2]);
            }
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
        if (!c->lambda->jumps.depth) {
            bs_compile_error_unexpected(c, &token);
        }

        bs_compile_block_drop(c, c->lambda->jumps.depth);
        bs_jumps_push(c->bs, &c->lambda->jumps, c->chunk->count);
        bs_chunk_push_op_int(c->bs, c->chunk, BS_OP_JUMP, 0);
    } break;

    case BS_TOKEN_CONTINUE: {
        if (!c->lambda->jumps.depth) {
            bs_compile_error_unexpected(c, &token);
        }

        bs_compile_block_drop(c, c->lambda->jumps.depth);
        bs_compile_jump_direct(c, BS_OP_JUMP, c->lambda->jumps.start);
    } break;

    case BS_TOKEN_FN:
        bs_compile_function(c, c->lambda->is_repl && c->lambda->depth == 1);
        break;

    case BS_TOKEN_PUB:
        if (c->lambda->depth != 1) {
            bs_lexer_error(&c->lexer, token.loc, "cannot define public values in local scope");
        }

        const Bs_Token_Type expected[] = {
            BS_TOKEN_FN,
            BS_TOKEN_VAR,
            BS_TOKEN_CLASS,
        };
        token = bs_lexer_one_of(&c->lexer, expected, bs_c_array_size(expected));

        if (token.type == BS_TOKEN_FN) {
            bs_compile_function(c, true);
        } else if (token.type == BS_TOKEN_VAR) {
            bs_compile_variable(c, true);
        } else if (token.type == BS_TOKEN_CLASS) {
            bs_compile_class(c, true);
        } else {
            assert(false && "unreachable");
        }
        break;

    case BS_TOKEN_VAR:
        bs_compile_variable(c, c->lambda->is_repl && c->lambda->depth == 1);
        break;

    case BS_TOKEN_CLASS:
        bs_compile_class(c, c->lambda->is_repl && c->lambda->depth == 1);
        break;

    case BS_TOKEN_RETURN:
        if (bs_lexer_peek_row(&c->lexer, &token) && bs_token_type_can_start(token.type)) {
            if (c->lambda->type == BS_LAMBDA_INIT) {
                const Bs_Loc loc = token.loc;
                const bool ok = token.type == BS_TOKEN_NIL;
                bs_lexer_unbuffer(&c->lexer);

                if (!ok) {
                    bs_lexer_error_full(
                        &c->lexer,
                        loc,

                        Bs_Sv_Static("When an initializer method explicitly returns 'nil', it "
                                     "indicates that the\n"
                                     "initialization failed due to some reason, and the site of "
                                     "the instantiation\n"
                                     "gets 'nil' as the result. This is not strictly OOP, but I "
                                     "missed the part where\n"
                                     "that's my problem."),

                        Bs_Sv_Static(
                            "class Log {\n"
                            "    init(path) {\n"
                            "        this.file = io.Writer(path)\n"
                            "        if !this.file {\n"
                            "            return nil // Failed to open log file\n"
                            "        }\n"
                            "    }\n"
                            "\n"
                            "    write(s) -> this.file.writeln(s)\n"
                            "}\n"
                            "\n"
                            "var log = Log(\"log.txt\")\n"
                            "if !log {\n"
                            "    panic() // Handle error\n"
                            "}\n"
                            "\n"
                            "log.write(\"Hello, world!\") // Or whatever you want to do\n"),

                        "can only explicity return 'nil' from an initializer method");
                }

                if (bs_lexer_peek_row(&c->lexer, &token) &&
                    bs_token_type_power(token.type) != BS_POWER_NIL) {
                    bs_compile_error_unexpected(c, &token);
                }

                c->class->can_fail = true;
                bs_chunk_push_op(c->bs, c->chunk, BS_OP_NIL);
            } else {
                bs_compile_expr(c, BS_POWER_SET);
            }
        } else {
            if (c->lambda->type == BS_LAMBDA_INIT) {
                bs_chunk_push_op_int(c->bs, c->chunk, BS_OP_LRECEIVER, 0);
            } else {
                bs_chunk_push_op(c->bs, c->chunk, BS_OP_NIL);
            }
        }

        bs_chunk_push_op(c->bs, c->chunk, BS_OP_RET);
        break;

    default:
        bs_lexer_buffer(&c->lexer, token);
        bs_compile_expr(c, BS_POWER_NIL);
        bs_chunk_push_op(c->bs, c->chunk, BS_OP_DROP);

        c->last_stmt_was_expr = (c->lambda->is_repl || c->lambda->is_meta) && c->lambda->depth == 1;
        break;
    }

    bs_compile_consume_eol(c);
}

Bs_Closure *bs_compile(
    Bs *bs, Bs_Sv path, Bs_Sv input, bool is_main, bool is_repl, bool is_meta, size_t module) {
    Bs_Compiler compiler = {
        .bs = bs,
        .is_main = is_main,
        .module = module,
    };

    Bs_Lambda *lambda = bs_lambda_new(BS_LAMBDA_FN, is_repl, is_meta);
    bs_compile_lambda_init(&compiler, lambda, path);
    bs_compile_block_init(&compiler);

    {
        // Own the path
        const Bs_Sv path = Bs_Sv(lambda->fn->name->data, lambda->fn->name->size);
        compiler.lexer = bs_lexer_new(path, input, &bs_config(bs)->error);
        compiler.lexer.is_meta = is_meta;
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

        bs_op_locs_free(compiler.bs, &compiler.locations);
        return NULL;
    }

    while (!bs_lexer_read(&compiler.lexer, BS_TOKEN_EOF)) {
        bs_compile_consume_eol(&compiler);
        if (bs_lexer_read(&compiler.lexer, BS_TOKEN_EOF)) {
            break;
        }

        compiler.last_stmt_was_expr = false;
        bs_compile_stmt(&compiler);
    }

    if ((is_repl || is_meta) && compiler.last_stmt_was_expr) {
        compiler.chunk->data[compiler.chunk->last] = BS_OP_RET;
    }

    Bs_Fn *fn = bs_compile_lambda_end(&compiler);
    fn->module = module;
    bs_lambda_free(compiler.bs, lambda);
    bs_op_locs_free(compiler.bs, &compiler.locations);
    return bs_closure_new(bs, fn);
}
