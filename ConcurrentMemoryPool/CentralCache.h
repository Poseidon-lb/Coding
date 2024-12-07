#pragma once

#include "Common.h"
#include "PageCache.h"

class CentralCache {
public:
    static CentralCache* getInstance() {
        return &instance;
    } 
    //获取一个非空的span
	Span* GetOneSpan(SpanList &list, size_t byte_size);
    //从中心缓存获取一定数量的对象给thread cache
	size_t FetchRangeObj(void*& start, void*& end, size_t n, size_t size);
    //将一定数量的对从threacache回收到central cache的span
	void ReleaseListToSpans(void* start, size_t byte_size);
private:
    CentralCache() {}
    CentralCache(const CentralCache&) = delete;
private:  
    SpanList _spanList[NFREELIST];
    static CentralCache instance;
};
