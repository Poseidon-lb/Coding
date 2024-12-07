#pragma once 

#include "Common.h"

class ThreadCache {
public:
    void *allocate(size_t size);
    void deAllocate(void *prt, size_t size);

    // //从中心缓存获取对象
    void* FetchFromCentralCache(size_t index, size_t size);
    // 释放内存时，链表过长, 回收内存返回中心缓存
    void ListTooLong(FreeList &list, size_t size);
private:      
    FreeList lists[NFREELIST];
};

