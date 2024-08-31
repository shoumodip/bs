#include "bs/hash.h"

static uint32_t bs_hash_string(const char *data, size_t size) {
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < size; i++) {
        hash ^= (uint8_t)data[i];
        hash *= 16777619;
    }
    return hash;
}

Entry *bs_entries_find_sv(Entry *entries, size_t capacity, Bs_Sv key, uint32_t *hash) {
    if (!capacity) {
        return NULL;
    }

    uint32_t index = (hash ? *hash : bs_hash_string(key.data, key.size)) % capacity;
    Entry *tombstone = NULL;

    while (true) {
        Entry *entry = &entries[index];
        if (entry->key) {
            if (bs_sv_eq(bs_sv_from_parts(entry->key->data, entry->key->size), key)) {
                return entry;
            }
        } else {
            if (entry->value.type == BS_VALUE_NIL) {
                return tombstone ? tombstone : entry;
            }

            if (!tombstone) {
                tombstone = entry;
            }
        }

        index = (index + 1) % capacity;
    }
}

Entry *bs_entries_find_str(Entry *entries, size_t capacity, Bs_Str *str) {
    if (!str->hashed) {
        str->hashed = true;
        str->hash = bs_hash_string(str->data, str->size);
    }

    return bs_entries_find_sv(
        entries, capacity, bs_sv_from_parts(str->data, str->size), &str->hash);
}
