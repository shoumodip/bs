#include "bs/object.h"

#define BS_MAP_MAX_LOAD 0.75

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
        index = ((Bs_Str *)key.as.object)->hash;
    } else {
        index = bs_hash_bytes(&key, sizeof(key));

        // TODO: temporary
        if (key.type == BS_VALUE_NUM) {
            printf("[DEBUG] Initial hash = %u\n", index);
        }
    }
    index = index & (capacity - 1);

    // TODO: temporary
    if (key.type == BS_VALUE_NUM) {
        printf("[DEBUG] Hash after mod = %u\n", index);
    }

    Bs_Entry *tombstone = NULL;
    while (true) {
        Bs_Entry *e = &entries[index];
        if (e->key.type == BS_VALUE_NIL) {
            if (e->value.type == BS_VALUE_NIL) {
                return tombstone ? tombstone : e;
            }

            if (!tombstone) {
                tombstone = e;
            }
        } else if (bs_value_equal(e->key, key)) {
            return e;
        }

        index = (index + 1) & (capacity - 1);
    }
}

Bs_Entry *bs_entries_find_sv(Bs_Entry *entries, size_t capacity, Bs_Sv key, uint32_t hash) {
    if (!capacity) {
        return NULL;
    }
    uint32_t index = hash & (capacity - 1);

    Bs_Entry *tombstone = NULL;
    while (true) {
        Bs_Entry *e = &entries[index];
        if (e->key.type == BS_VALUE_NIL) {
            if (e->value.type == BS_VALUE_NIL) {
                return tombstone ? tombstone : e;
            }

            if (!tombstone) {
                tombstone = e;
            }
        } else if (e->key.type == BS_VALUE_OBJECT && e->key.as.object->type == BS_OBJECT_STR) {
            const Bs_Str *str = (const Bs_Str *)e->key.as.object;
            if (bs_sv_eq(key, Bs_Sv(str->data, str->size))) {
                return e;
            }
        }

        index = (index + 1) & (capacity - 1);
    }
}

void bs_map_free(Bs *bs, Bs_Map *m) {
    bs_realloc(bs, m->data, sizeof(*m->data) * m->capacity, 0);
    memset(m, 0, sizeof(*m));
}

bool bs_map_remove(Bs *bs, Bs_Map *m, Bs_Value key) {
    if (!m->length) {
        return false;
    }

    Bs_Entry *entry = bs_entries_find(m->data, m->capacity, key);
    if (!entry || entry->key.type == BS_VALUE_NIL) {
        return false;
    }

    m->length--;
    entry->key = bs_value_nil;
    entry->value = bs_value_bool(true);
    return true;
}

bool bs_map_get(Bs *bs, Bs_Map *m, Bs_Value key, Bs_Value *value) {
    if (!m->length) {
        return false;
    }

    Bs_Entry *entry = bs_entries_find(m->data, m->capacity, key);
    if (entry->key.type == BS_VALUE_NIL) {
        return false;
    }

    if (value) {
        *value = entry->value;
    }
    return true;
}

static void bs_map_grow(Bs *bs, Bs_Map *m, size_t capacity) {
    const size_t size = sizeof(Bs_Entry) * capacity;

    Bs_Entry *entries = bs_realloc(bs, NULL, 0, size);
    memset(entries, 0, size);

    size_t count = 0;
    for (size_t i = 0; i < m->capacity; i++) {
        Bs_Entry *src = &m->data[i];
        if (src->key.type == BS_VALUE_NIL) {
            continue;
        }

        Bs_Entry *dst = bs_entries_find(entries, capacity, src->key);
        dst->key = src->key;
        dst->value = src->value;
        count++;
    }

    bs_map_free(bs, m);
    m->data = entries;
    m->count = count;
    m->capacity = capacity;
}

bool bs_map_set(Bs *bs, Bs_Map *m, Bs_Value key, Bs_Value value) {
    if (m->count >= m->capacity * BS_MAP_MAX_LOAD) {
        bs_map_grow(bs, m, m->capacity ? m->capacity * 2 : BS_DA_INIT_CAP);
    }
    Bs_Entry *entry = bs_entries_find(m->data, m->capacity, key);

    bool is_new = entry->key.type == BS_VALUE_NIL;
    if (is_new && entry->value.type == BS_VALUE_NIL) {
        m->count++;
        m->length++;
    }

    entry->key = key;
    entry->value = value;
    return is_new;
}

void bs_map_copy(Bs *bs, Bs_Map *dst, const Bs_Map *src) {
    for (size_t i = 0; i < src->capacity; i++) {
        const Bs_Entry *entry = &src->data[i];
        if (entry->key.type != BS_VALUE_NIL) {
            bs_map_set(bs, dst, entry->key, entry->value);
        }
    }
}
