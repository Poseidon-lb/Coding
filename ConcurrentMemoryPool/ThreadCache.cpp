#include "ThreadCache.h"
#include "CentralCache.h"

// 线程局部存储TLS(Thread Local Storage)
// 每个被thread_local声明的变量在各自线程都是独立的
thread_local ThreadCache *pTLSThreadCache = nullptr;

void* ThreadCache::FetchFromCentralCache(size_t index, size_t size) {
    //慢开始调节算法
	//1.开始不会向central cache申请太多，因为太多了可能浪费也多
	//2.下次每次申请都会多申请一个直到上限，上限即向central cache申请的空间
	//3.size越大，向central cache要的也就越小
	//4.size 越小，向central cache要的也就越大，但是受MaxSize()一定的约束
    size_t batchNum = std::min(SizeClass::NumMoveSize(size), lists[index].maxSize());
    if (batchNum == lists[index].maxSize()) {
        lists[index].add_maxSize(1);
    }
    void *start = nullptr, *end = nullptr;
    //实际从central cache申请到空间节点的数量
	size_t actualNum = CentralCache::getInstance()->FetchRangeObj(start, end, batchNum, size);
    assert(actualNum > 0);
    lists[index].push_range(start, end, actualNum);
    void *node = lists[index].front();
    lists[index].pop_front();
    return node;
}

void* ThreadCache::allocate(size_t size) {
    assert(size <= MAX_BYTES);
    size_t index = SizeClass::Index(size);
    if (!lists[index].empty()) {
        void *node = lists[index].front();
        lists[index].pop_front();
        return node;
    } else {
        return FetchFromCentralCache(index, size);
    }
}

void ThreadCache::ListTooLong(FreeList &list, size_t size) {
    void *start = nullptr, *end = nullptr;
    list.pop_range(start, end, list.maxSize());
    CentralCache::getInstance()->ReleaseListToSpans(start, size);
}

void ThreadCache::deAllocate(void *ptr, size_t size) {
    assert(size <= MAX_BYTES);
    assert(ptr);
    size_t index = SizeClass::Index(size);
    lists[index].push_front(ptr);
    // 如果借出去的全都还回来了，就暂时用不上，还给上一层
    if (lists[index].size() >= lists[index].maxSize()) {
        ListTooLong(lists[index], size);
    }
}

