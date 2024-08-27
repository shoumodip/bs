#ifndef HASH_H
#define HASH_H

#include "object.h"

Entry *entries_find_sv(Entry *entries, size_t capacity, SV key, uint32_t *hash);
Entry *entries_find_str(Entry *entries, size_t capacity, ObjectStr *str);

#endif // HASH_H
