#ifndef BS_HASH_H
#define BS_HASH_H

#include "object.h"

Entry *bs_entries_find_sv(Entry *entries, size_t capacity, Bs_Sv key, uint32_t *hash);
Entry *bs_entries_find_str(Entry *entries, size_t capacity, Bs_Str *str);

#endif // BS_HASH_H
