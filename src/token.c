#include <assert.h>

#include "token.h"

static_assert(COUNT_TOKENS == 43, "Update token_type_name()");
const char *token_type_name(TokenType type, bool extended) {
    switch (type) {
    case TOKEN_EOF:
        return "end of file";

    case TOKEN_EOL:
        return extended ? "'fr'" : "';'";

    case TOKEN_DOT:
        return "'.'";

    case TOKEN_COMMA:
        return "','";

    case TOKEN_NIL:
        return extended ? "'bruh'" : "'nil'";

    case TOKEN_NUM:
        return "number";

    case TOKEN_STR:
        return "string";

    case TOKEN_TRUE:
        return extended ? "'nocap'" : "'true'";

    case TOKEN_FALSE:
        return extended ? "'cap'" : "'false'";

    case TOKEN_IDENT:
        return "identifier";

    case TOKEN_NATIVE:
        return "'@'";

    case TOKEN_LPAREN:
        return "'('";

    case TOKEN_RPAREN:
        return "')'";

    case TOKEN_LBRACE:
        return "'{'";

    case TOKEN_RBRACE:
        return "'}'";

    case TOKEN_LBRACKET:
        return "'['";

    case TOKEN_RBRACKET:
        return "']'";

    case TOKEN_ADD:
        return "'+'";

    case TOKEN_SUB:
        return "'-'";

    case TOKEN_MUL:
        return "'*'";

    case TOKEN_DIV:
        return "'/'";

    case TOKEN_OR:
        return "'or'";

    case TOKEN_AND:
        return "'and'";

    case TOKEN_NOT:
        return extended ? "'nah'" : "'!'";

    case TOKEN_GT:
        return "'>'";

    case TOKEN_GE:
        return "'>='";

    case TOKEN_LT:
        return "'<'";

    case TOKEN_LE:
        return "'<='";

    case TOKEN_EQ:
        return "'=='";

    case TOKEN_NE:
        return "'!='";

    case TOKEN_LEN:
        return extended ? "'thicc'" : "'len'";

    case TOKEN_JOIN:
        return "'..'";

    case TOKEN_IMPORT:
        return extended ? "'redpill'" : "'import'";

    case TOKEN_SET:
        return extended ? "'be'" : "'='";

    case TOKEN_IF:
        return extended ? "'ayo'" : "'if'";

    case TOKEN_ELSE:
        return extended ? "'sike'" : "'else'";

    case TOKEN_FOR:
        return extended ? "'yall'" : "'for'";

    case TOKEN_WHILE:
        return extended ? "'yolo'" : "'while'";

    case TOKEN_FN:
        return extended ? "'lit'" : "'fn'";

    case TOKEN_PUB:
        return extended ? "'fam'" : "'pub'";

    case TOKEN_VAR:
        return extended ? "'mf'" : "'var'";

    case TOKEN_RETURN:
        return extended ? "'bet'" : "'return'";

    case TOKEN_PRINT:
        return extended ? "'yap'" : "'print'";

    default:
        assert(false && "unreachable");
    }
}
