#include "ChannelMap.h"

struct ChannelMap* ChannelMapInit(int size) {
    struct ChannelMap* map = (struct ChannelMap*)malloc(sizeof (struct ChannelMap));
    map->size = size;
    map->list = (struct Channel**)malloc(size * sizeof (struct Channel*));
    return map;
}

void ChannelMapClear(struct ChannelMap* map) {
    if (map == NULL) {
        return;
    }
    for (int i = 0; i < map->size; i ++) {
        if (map->list[i] != NULL) {
            free(map->list[i]);
        }
    }
    free(map->list);
    map->list = NULL;
    map->size = 0;
}

bool makeMapRoom(struct ChannelMap* map, int newSize, int unitSize) {
    if (map->size >= newSize) {
        return true;
    }
    int curSize = map->size;
    while (curSize < newSize) {
        curSize *= 2;
    }
    // 扩容realloc 后面有就追加，没有就重新找一块，再将原本的拷贝过去
    struct Channel** nlist = realloc(map->list, curSize * unitSize);
    if (nlist == NULL) {
        return false;
    }
    map->list = nlist;
    memset(&map->list[map->size], 0, unitSize * sizeof (curSize - map->size));
    map->size = curSize;
    return true;
}