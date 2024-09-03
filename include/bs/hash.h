#ifndef BS_HASH_H
#define BS_HASH_H

#include "object.h"

Bs_Entry *bs_entries_find_sv(Bs_Entry *entries, size_t capacity, Bs_Sv key, uint32_t *hash);
Bs_Entry *bs_entries_find_str(Bs_Entry *entries, size_t capacity, Bs_Str *str);

#endif // BS_HASH_H
