#ifndef BS_MAP_H
#define BS_MAP_H

#include "vm.h"

typedef struct {
    Bs_Value key;
    Bs_Value value;
} Bs_Entry;

typedef struct {
    Bs_Entry *data;
    size_t count;
    size_t length;
    size_t capacity;
} Bs_Map;

void bs_map_free(Bs *bs, Bs_Map *map);
bool bs_map_remove(Bs *bs, Bs_Map *map, Bs_Value key);

bool bs_map_get(Bs *bs, Bs_Map *map, Bs_Value key, Bs_Value *value);
bool bs_map_set(Bs *bs, Bs_Map *map, Bs_Value key, Bs_Value value);

#endif // BS_MAP_H
