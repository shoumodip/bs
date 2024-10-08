#ifndef BS_MAP_H
#define BS_MAP_H

#include "vm.h"

uint32_t bs_hash_bytes(const void *data, size_t size);
uint32_t bs_hash_uint64(uint64_t hash);
uint32_t bs_hash_value(Bs_Value key);

typedef struct {
    Bs_Value key;
    Bs_Value value;
} Bs_Entry;

Bs_Entry *bs_entries_find(Bs_Entry *entries, size_t capacity, Bs_Value key);
Bs_Entry *bs_entries_find_sv(Bs_Entry *entries, size_t capacity, Bs_Sv key, uint32_t hash);

struct Bs_Map {
    Bs_Entry *data;
    size_t count;
    size_t length;
    size_t capacity;
};

void bs_map_free(Bs *bs, Bs_Map *map);
bool bs_map_remove(Bs *bs, Bs_Map *map, Bs_Value key);

bool bs_map_get(Bs *bs, Bs_Map *map, Bs_Value key, Bs_Value *value);
bool bs_map_set(Bs *bs, Bs_Map *map, Bs_Value key, Bs_Value value);

void bs_map_copy(Bs *bs, Bs_Map *dst, const Bs_Map *src);

#endif // BS_MAP_H
