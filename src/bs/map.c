#include "bs/map.h"
#include "bs/hash.h"

#define BS_MAP_MAX_LOAD 0.75

void bs_map_free(Bs *bs, Bs_Map *m) {
    bs_realloc(bs, m->data, sizeof(*m->data) * m->capacity, 0);
    memset(m, 0, sizeof(*m));
}

bool bs_map_remove(Bs *bs, Bs_Map *m, Bs_Value key) {
    if (!m->count) {
        return false;
    }

    Bs_Entry *entry = bs_entries_find(m->data, m->capacity, key);
    if (!entry) {
        return false;
    }

    if (entry->key.type == BS_VALUE_NIL && entry->value.type == BS_VALUE_BOOL &&
        entry->value.as.boolean == true) {
        return false;
    }

    m->length--;
    entry->key = bs_value_nil;
    entry->value = bs_value_bool(true);
    return true;
}

bool bs_map_get(Bs *bs, Bs_Map *m, Bs_Value key, Bs_Value *value) {
    if (!m->count) {
        return false;
    }

    Bs_Entry *entry = bs_entries_find(m->data, m->capacity, key);
    if (entry->key.type == BS_VALUE_NIL) {
        return false;
    }

    *value = entry->value;
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
