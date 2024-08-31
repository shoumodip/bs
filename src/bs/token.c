#include <assert.h>

#include "bs/token.h"

static_assert(BS_COUNT_TOKENS == 43, "Update bs_token_type_name()");
const char *bs_token_type_name(Bs_Token_Type type, bool extended) {
    switch (type) {
    case BS_TOKEN_EOF:
        return "end of file";

    case BS_TOKEN_EOL:
        return extended ? "'fr'" : "';'";

    case BS_TOKEN_DOT:
        return "'.'";

    case BS_TOKEN_COMMA:
        return "','";

    case BS_TOKEN_NIL:
        return extended ? "'bruh'" : "'nil'";

    case BS_TOKEN_NUM:
        return "number";

    case BS_TOKEN_STR:
        return "string";

    case BS_TOKEN_CORE:
        return "'@'";

    case BS_TOKEN_TRUE:
        return extended ? "'nocap'" : "'true'";

    case BS_TOKEN_FALSE:
        return extended ? "'cap'" : "'false'";

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

    case BS_TOKEN_OR:
        return "'or'";

    case BS_TOKEN_AND:
        return "'and'";

    case BS_TOKEN_NOT:
        return extended ? "'nah'" : "'!'";

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
        return extended ? "'thicc'" : "'len'";

    case BS_TOKEN_JOIN:
        return "'..'";

    case BS_TOKEN_IMPORT:
        return extended ? "'redpill'" : "'import'";

    case BS_TOKEN_SET:
        return extended ? "'be'" : "'='";

    case BS_TOKEN_IF:
        return extended ? "'ayo'" : "'if'";

    case BS_TOKEN_ELSE:
        return extended ? "'sike'" : "'else'";

    case BS_TOKEN_FOR:
        return extended ? "'yall'" : "'for'";

    case BS_TOKEN_WHILE:
        return extended ? "'yolo'" : "'while'";

    case BS_TOKEN_FN:
        return extended ? "'lit'" : "'fn'";

    case BS_TOKEN_PUB:
        return extended ? "'fam'" : "'pub'";

    case BS_TOKEN_VAR:
        return extended ? "'mf'" : "'var'";

    case BS_TOKEN_RETURN:
        return extended ? "'bet'" : "'return'";

    case BS_TOKEN_PRINT:
        return extended ? "'yap'" : "'print'";

    default:
        assert(false && "unreachable");
    }
}