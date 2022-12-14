#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "hashmap.h"
#include "move.h"

HashMap* hashmap_alloc(int size) {
    HashMap* hashmap = (HashMap*) malloc(sizeof(HashMap));
    hashmap->size = 1 << size;
    hashmap->data = calloc(hashmap->size, sizeof(Item));
    return hashmap;
}

void hashmap_free(HashMap* hashmap) {
    free(hashmap->data);
    free(hashmap);
}

void hashmap_clear(HashMap* hashmap) {
    memset(hashmap->data, 0, hashmap->size * sizeof(Item));
}

void hashmap_set(HashMap* hashmap, uint64_t key, int value, int depth, int flag) {
    Item* item = &hashmap->data[(key >> KEY_OFFSET) & (hashmap->size - 1)];
    if (depth >= item->depth) {
        item->key = key;
        item->value = value;
        item->depth = depth;
        item->flag = flag;
    }
}

int hashmap_get(HashMap* hashmap, uint64_t key, int depth, int* ret) {
    Item* item = &hashmap->data[(key >> KEY_OFFSET) & (hashmap->size - 1)];
    if (depth <= item->depth && key == item->key) {
        *ret = item->value;
        return item->flag;
    }
    return 0;
}
