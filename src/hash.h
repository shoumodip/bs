#ifndef HASH_H
#define HASH_H

#include "object.h"

uint32_t hash_string(const char *data, size_t size);
Entry *entries_find(Entry *entries, size_t capacity, ObjectStr *key);

#endif // HASH_H
