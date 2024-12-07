#pragma once
#define _GNU_SOURCE
#include "Channel.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>

struct ChannelMap {
    int size;                   // 数组的大小
    // struct Channel* lsit[]
    struct Channel** list;
};

// 初始化ChannelMap
struct ChannelMap* ChannelMapInit(int size);

// 清空ChannelMap
void ChannelMapClear(struct ChannelMap* map);

// 重新分配内存空间
bool makeMapRoom(struct ChannelMap* map, int newSize, int unitSize);