#include "PageCache.h"

PageCache PageCache::instance;

Span *PageCache::NewSpan(size_t k) {
    assert(k > 0);
    //大于128页直接向堆申请
    if (k >= NPAGES) {  //大于page cache的最大span的页
        void *ptr = SystemAlloc(k);
        Span *span = _spanPool.New();
        span->_PageId = (PAGE_ID)ptr >> PAGE_SHIFT;
        span->n = k;
        _idSpanMap.set(span->_PageId, span);
        return span;
    }

    // 分出去的时候就设置哪页属于哪个span
    // 还回来的时候就根据小数据的页号查找对应span
    if (!_spanLists[k].empty()) {
        Span *kspan = _spanLists[k].begin();
        _spanLists[k].pop_front();
        for (PAGE_ID i = 0; i < kspan->n; i ++) {
            _idSpanMap.set(kspan->_PageId + i, kspan);
        }
        return kspan;
    }
    //检查一下后面的桶里面有没有span,如果有可以把他进行切分
    for (int i = k + 1; i < NPAGES; i ++) {
        if (!_spanLists[i].empty()) {
            Span *nSpan = _spanLists[i].begin();
            _spanLists[i].pop_front();
            Span *kSpan = _spanPool.New();
            //在nSpan的头部切一个k页空间的页
			//K页的span返回
			//剩下的nspan要重新挂到合适的位置
            kSpan->_PageId = nSpan->_PageId;
            kSpan->n = k;
            nSpan->_PageId += k;    // 这块大内存的起始地址往后移动k
            nSpan->n -= k;
            _spanLists[nSpan->n].push_front(nSpan);
            //存储nSpan的首尾页跟nSpan的映射，方便回收page cache回收内存时进行合并查找
            _idSpanMap.set(nSpan->_PageId, nSpan);
            _idSpanMap.set(nSpan->_PageId + nSpan->n - 1, nSpan);
            //建立页号和span的映射，便于central cache回收时，查找对应的span
            for (PAGE_ID i = 0; i < kSpan->n; i ++) {
                _idSpanMap.set(kSpan->_PageId + i, kSpan);
            }
            return kSpan;
        }
    }

	//此时说明Page Cache目前没有k页的span,也没有比k页大的span切分
	//找堆要一个128页的span
    Span *bigSpan = _spanPool.New();
    void* ptr = SystemAlloc(NPAGES - 1);
    bigSpan->n = NPAGES - 1;
    bigSpan->_PageId = (PAGE_ID)ptr >> PAGE_SHIFT;

    _spanLists[bigSpan->n].push_front(bigSpan);
    return NewSpan(k);
}

//获取从对象到span的映射
Span *PageCache::MapObjectToSpan(void *obj) {
    //先计算页号
	PAGE_ID id = ((PAGE_ID)obj >> PAGE_SHIFT);

    auto ret = (Span*)_idSpanMap.get(id);
	assert(ret != nullptr);
	return ret;
}

//释放空闲span到PageCache,合并相邻的span
void PageCache::ReleaseSpanToPageCache(Span *span) {
    //大于128页直接还给堆
    if (span->n > NPAGES - 1)
	{
		void* ptr = (void*)(span->_PageId << PAGE_SHIFT);
		SystemFree(ptr);
		//delete span;
		_spanPool.Delete(span);
		return;
	}
    //对span前后的页，尝试进行合并，缓解内存碎片的问题
    while (1) {
        PAGE_ID prevId = span->_PageId - 1;
        auto ret = (Span*)_idSpanMap.get(prevId);
		if (ret == nullptr) {
			break;
		}
        //前面相邻页的span在使用，不合并
		Span* prevSpan = ret;
		//前面的span在被使用，不合并
		if (prevSpan->_isUse == true) {
			break;
		}
		//合并后得分span空间超过限制，不合并
		if (prevSpan->n + span->n > NPAGES - 1) {
			break;
		}
		span->_PageId = prevSpan->_PageId;
		span->n += prevSpan->n;

		_spanLists[prevSpan->n].erase(prevSpan);
		//delete prevSpan;
		_spanPool.Delete(prevSpan);
    }

    //向后合并
	while (1)
	{
		PAGE_ID nextId = span->_PageId + span->n;
		//auto ret = _idSpanMap.find(nextId);
		// 后面的页号没有，不合并了
		//if (ret != _idSpanMap.end())
		//{
		//	break;
		//}
		auto ret = (Span*)_idSpanMap.get(nextId);
		if (ret == nullptr) {
			break;
		}
		//后面相邻页的span在使用，不合并
		Span* nextSpan = ret;
		//前面的span在被使用，不合并
		if (nextSpan->_isUse == true) {
			break;
		}
		//合并后得分span空间超过限制，不合并
		if (nextSpan->n + span->n > NPAGES - 1) {
			break;
		}
		//向后合并不需要更新pageid
		span->n += nextSpan->n;
		_spanLists[nextSpan->n].erase(nextSpan);
		//delete nextSpan;
		_spanPool.Delete(nextSpan);
	}
    _spanLists[span->n].push_front(span);
    span->_isUse = false;
    _idSpanMap.set(span->_PageId, span);
    _idSpanMap.set(span->_PageId + span->n - 1, span);
}


