#include "CentralCache.h"

CentralCache CentralCache::instance;

//获取一个非空的span
Span *CentralCache::GetOneSpan(SpanList &list, size_t size) {
    Span *it = list.begin();
    while (it != list.end()) {
        if (it->freeList != nullptr) {
            return it;
        }
        it = it->next;
    }
    //先把central cache的桶锁解开，这样如果其它线程释放内存回来，不会阻塞
    list.unlock();
    //到这说明central cache的spanlist中没有合适的span，需要向page cache申请
    PageCache::getInstance()->lock();
    Span *span = PageCache::getInstance()->NewSpan(SizeClass::NumMovePage(size));
    span->_isUse = true;
    span->objSize = size;
    PageCache::getInstance()->unlock();

    //对获取的span进行切分，不需要加锁，因为此时其它线程访问访问的不是同一个span 
	//计算span大块内存的起始地址和大块内存的大小（字节）
    char *start = (char *)(span->_PageId << PAGE_SHIFT);
    size_t bytes = span->n << PAGE_SHIFT;
    char *end = start + bytes;

    //把大块内存切成自由链表挂起来
	//1.先切一块下来去做头，方便尾插
    span->freeList = start;
    start += size;
    void *tail = span->freeList;
    while (start < end) {
        tail = _next(tail) = start;
        start += size;
    }
    _next(end) = nullptr;
    //把切好的span挂到central cache的桶里面，需要加锁
    list.lock();
    list.push_front(span);
    //list.unlock();
    //list.lock();
    return span;
}

//从中心缓存获取一定数量的对象给thread cache
size_t CentralCache::FetchRangeObj(void *&start, void *&end, size_t n, size_t size) {
    size_t index = SizeClass::Index(size);
    // 给桶上锁
    _spanList[index].lock();
    Span *span = GetOneSpan(_spanList[index], size);
    assert(span);
    assert(span->freeList);
    start = end = span->freeList;
    size_t num = 0;
    for (int i = 0; i < n - 1 && end != nullptr; i ++) {
        end = _next(end);
        num++;   
    }
    span->useCount += num;
    span->freeList = _next(end);
    _next(end) = nullptr;
    _spanList[index].unlock();
    return num;
}
//将一定数量的内存对像从threacache回收到central cache的span
void CentralCache::ReleaseListToSpans(void *start, size_t size) {
    size_t index = SizeClass::Index(size);
    _spanList[index].lock();
    
    while (start) {
        void *next = _next(start);
        Span *span = PageCache::getInstance()->MapObjectToSpan(start);
        _next(start) = span->freeList;
        span->freeList = start;
        // 每个小数据不一定是同一个span，所以换回去一个判断一个
        if (--span->useCount == 0) {
            //span切分出去的小块内存对象都释放了
			//可以回收给page cache，page cache可以尝试合并前后页
            _spanList[index].erase(span);
            span->freeList = nullptr;
            span->next = span->prev = nullptr;
            //释放span给page cache时，使用page cache的锁就可以了
			//释放桶锁，其它线程此时可以向central cache申请或者释放span
            _spanList[index].unlock();
            PageCache::getInstance()->lock();
            PageCache::getInstance()->ReleaseSpanToPageCache(span);
            PageCache::getInstance()->unlock();
            // 回来继续使用，重新上锁
            _spanList[index].lock();
        }
        start = next;
    }

    _spanList[index].unlock();
}
