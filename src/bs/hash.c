#include "bs/hash.h"

uint32_t bs_hash_bytes(const void *data, size_t size) {
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < size; i++) {
        hash ^= ((uint8_t *)data)[i];
        hash *= 16777619;
    }
    return hash;
}

Bs_Entry *bs_entries_find(Bs_Entry *entries, size_t capacity, Bs_Value key) {
    if (!capacity) {
        return NULL;
    }

    uint32_t index;
    if (key.type == BS_VALUE_OBJECT && key.as.object->type == BS_OBJECT_STR) {
        Bs_Str *str = (Bs_Str *)key.as.object;
        if (!str->hashed) {
            str->hashed = true;
            str->hash = bs_hash_bytes(str->data, str->size);
        }

        index = str->hash;
    } else {
        index = bs_hash_bytes(&key, sizeof(key));
    }
    index = index & (capacity - 1);

    Bs_Entry *tombstone = NULL;
    while (true) {
        Bs_Entry *entry = &entries[index];
        if (entry->key.type != BS_VALUE_NIL) {
            if (bs_value_equal(entry->key, key)) {
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

        index = (index + 1) & (capacity - 1);
    }
}

Bs_Entry *bs_entries_find_sv(Bs_Entry *entries, size_t capacity, Bs_Sv key) {
    if (!capacity) {
        return NULL;
    }
    uint32_t index = bs_hash_bytes(key.data, key.size) & (capacity - 1);

    Bs_Entry *tombstone = NULL;
    while (true) {
        Bs_Entry *entry = &entries[index];
        if (entry->key.type != BS_VALUE_NIL) {
            if (entry->key.type == BS_VALUE_OBJECT && entry->key.as.object->type == BS_OBJECT_STR) {
                const Bs_Str *str = (const Bs_Str *)entry->key.as.object;
                if (bs_sv_eq(key, Bs_Sv(str->data, str->size))) {
                    return entry;
                }
            }
        } else {
            if (entry->value.type == BS_VALUE_NIL) {
                return tombstone ? tombstone : entry;
            }

            if (!tombstone) {
                tombstone = entry;
            }
        }

        index = (index + 1) & (capacity - 1);
    }
}
