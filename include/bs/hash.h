#ifndef BS_HASH_H
#define BS_HASH_H

#include "object.h"

uint32_t bs_hash_bytes(const void *data, size_t size);

Bs_Entry *bs_entries_find(Bs_Entry *entries, size_t capacity, Bs_Value key);
Bs_Entry *bs_entries_find_sv(Bs_Entry *entries, size_t capacity, Bs_Sv key);

#endif // BS_HASH_H
