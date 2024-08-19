#include "token.h"

static_assert(COUNT_TOKENS == 12, "Update token_type_name()");
const char *token_type_name(TokenType type) {
    switch (type) {
    case TOKEN_EOF:
        return "end of file";

    case TOKEN_EOL:
        return "';'";

    case TOKEN_NIL:
        return "'nil'";

    case TOKEN_NUM:
        return "number";

    case TOKEN_TRUE:
    case TOKEN_FALSE:
        return "boolean";

    case TOKEN_IDENT:
        return "identifier";

    case TOKEN_ADD:
        return "'+'";

    case TOKEN_SUB:
        return "'-'";

    case TOKEN_MUL:
        return "'*'";

    case TOKEN_DIV:
        return "'/'";

    case TOKEN_NOT:
        return "'!'";

    default:
        assert(false && "unreachable");
    }
}
