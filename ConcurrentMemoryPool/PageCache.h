#pragma once

#include "Common.h"
#include "PageMap.h"
#include "ObjectPool.h"

class PageCache {
public:
    static PageCache* getInstance() {
        return &instance;
    }
	//获取从对象到span的映射
	Span* MapObjectToSpan(void* obj);
    //释放空闲span到PageCache,合并相邻的span
	void ReleaseSpanToPageCache(Span* span);
    //获取一个k页的span
	Span* NewSpan(size_t k);
    //整个哈希表的锁
    void lock() {
        _mutex.lock();
    }
    void unlock() {
        _mutex.unlock();
    }
private:
    PageCache() {}
    PageCache(const PageCache&) = delete;
    std::mutex _mutex;
    SpanList _spanLists[NPAGES];
    ObjectPool<Span> _spanPool;
    //std::unordered_map<PAGE_ID, Span*> _idSpanMap;
	TCMalloc_PageMap1<32 - PAGE_SHIFT> _idSpanMap; 
    static PageCache instance;
};