#include <assert.h>

#include "bs/token.h"

static_assert(BS_COUNT_TOKENS == 77, "Update bs_token_type_name()");
const char *bs_token_type_name(Bs_Token_Type type) {
    switch (type) {
    case BS_TOKEN_EOF:
        return "end of file";

    case BS_TOKEN_EOL:
        return "';'";

    case BS_TOKEN_DOT:
        return "'.'";

    case BS_TOKEN_ARROW:
        return "'->'";

    case BS_TOKEN_COMMA:
        return "','";

    case BS_TOKEN_SPREAD:
        return "'..'";

    case BS_TOKEN_NIL:
        return "'nil'";

    case BS_TOKEN_NUM:
        return "number";

    case BS_TOKEN_STR:
    case BS_TOKEN_ISTR:
    case BS_TOKEN_RSTR:
        return "string";

    case BS_TOKEN_TRUE:
        return "'true'";

    case BS_TOKEN_FALSE:
        return "'false'";

    case BS_TOKEN_IDENT:
        return "identifier";

    case BS_TOKEN_LPAREN:
        return "'('";

    case BS_TOKEN_RPAREN:
        return "')'";

    case BS_TOKEN_LBRACE:
        return "'{'";

    case BS_TOKEN_RBRACE:
        return "'}'";

    case BS_TOKEN_LBRACKET:
        return "'['";

    case BS_TOKEN_RBRACKET:
        return "']'";

    case BS_TOKEN_ADD:
        return "'+'";

    case BS_TOKEN_SUB:
        return "'-'";

    case BS_TOKEN_MUL:
        return "'*'";

    case BS_TOKEN_DIV:
        return "'/'";

    case BS_TOKEN_MOD:
        return "'%'";

    case BS_TOKEN_BOR:
        return "'|'";

    case BS_TOKEN_BAND:
        return "'&'";

    case BS_TOKEN_BXOR:
        return "'^'";

    case BS_TOKEN_BNOT:
        return "'~'";

    case BS_TOKEN_LOR:
        return "'||'";

    case BS_TOKEN_LAND:
        return "'&&'";

    case BS_TOKEN_LNOT:
        return "'!'";

    case BS_TOKEN_SHL:
        return "'<<'";

    case BS_TOKEN_SHR:
        return "'>>'";

    case BS_TOKEN_GT:
        return "'>'";

    case BS_TOKEN_GE:
        return "'>='";

    case BS_TOKEN_LT:
        return "'<'";

    case BS_TOKEN_LE:
        return "'<='";

    case BS_TOKEN_EQ:
        return "'=='";

    case BS_TOKEN_NE:
        return "'!='";

    case BS_TOKEN_LEN:
        return "'len'";

    case BS_TOKEN_JOIN:
        return "'$'";

    case BS_TOKEN_PANIC:
        return "'panic'";

    case BS_TOKEN_ASSERT:
        return "'assert'";

    case BS_TOKEN_DELETE:
        return "'delete'";

    case BS_TOKEN_IMPORT:
        return "'import'";

    case BS_TOKEN_TYPEOF:
        return "'typeof'";

    case BS_TOKEN_SET:
        return "'='";

    case BS_TOKEN_ADD_SET:
        return "'+='";

    case BS_TOKEN_SUB_SET:
        return "'-='";

    case BS_TOKEN_MUL_SET:
        return "'*='";

    case BS_TOKEN_DIV_SET:
        return "'/='";

    case BS_TOKEN_MOD_SET:
        return "'%='";

    case BS_TOKEN_BOR_SET:
        return "'|='";

    case BS_TOKEN_BAND_SET:
        return "'&='";

    case BS_TOKEN_BXOR_SET:
        return "'^='";

    case BS_TOKEN_SHL_SET:
        return "'<<='";

    case BS_TOKEN_SHR_SET:
        return "'>>='";

    case BS_TOKEN_JOIN_SET:
        return "'$='";

    case BS_TOKEN_IF:
        return "'if'";

    case BS_TOKEN_THEN:
        return "'then'";

    case BS_TOKEN_ELSE:
        return "'else'";

    case BS_TOKEN_MATCH:
        return "'match'";

    case BS_TOKEN_IN:
        return "'in'";

    case BS_TOKEN_IS:
        return "'is'";

    case BS_TOKEN_FOR:
        return "'for'";

    case BS_TOKEN_WHILE:
        return "'while'";

    case BS_TOKEN_BREAK:
        return "'break'";

    case BS_TOKEN_CONTINUE:
        return "'continue'";

    case BS_TOKEN_FN:
        return "'fn'";

    case BS_TOKEN_PUB:
        return "'pub'";

    case BS_TOKEN_VAR:
        return "'var'";

    case BS_TOKEN_RETURN:
        return "'return'";

    case BS_TOKEN_THIS:
        return "'this'";

    case BS_TOKEN_SUPER:
        return "'super'";

    case BS_TOKEN_CLASS:
        return "'class'";

    case BS_TOKEN_IS_MAIN_MODULE:
        return "'is_main_module'";

    default:
        assert(false && "unreachable");
    }
}
