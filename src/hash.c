#include "hash.h"

static uint32_t hash_string(const char *data, size_t size) {
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < size; i++) {
        hash ^= (uint8_t)data[i];
        hash *= 16777619;
    }
    return hash;
}

Entry *entries_find_sv(Entry *entries, size_t capacity, SV key, uint32_t *hash) {
    if (!capacity) {
        return NULL;
    }

    uint32_t index = (hash ? *hash : hash_string(key.data, key.size)) % capacity;
    Entry *tombstone = NULL;

    while (true) {
        Entry *entry = &entries[index];
        if (entry->key) {
            if (sv_eq(sv_from_parts(entry->key->data, entry->key->size), key)) {
                return entry;
            }
        } else {
            if (entry->value.type == VALUE_NIL) {
                return tombstone ? tombstone : entry;
            }

            if (!tombstone) {
                tombstone = entry;
            }
        }

        index = (index + 1) % capacity;
    }
}

Entry *entries_find_str(Entry *entries, size_t capacity, ObjectStr *str) {
    if (!str->hashed) {
        str->hashed = true;
        str->hash = hash_string(str->data, str->size);
    }

    return entries_find_sv(entries, capacity, sv_from_parts(str->data, str->size), &str->hash);
}
