#pragma once
#include <unistd.h>
#include <bits/stdc++.h>

using PAGE_ID = size_t;


// 哈希表的大小
static constexpr size_t MAX_BYTES = 256 * 1024;
static constexpr size_t NFREELIST  = 208;
static constexpr size_t NPAGES = 129;
static constexpr size_t PAGE_SHIFT = 13;

inline static void* SystemAlloc(size_t size) {
    void *ptr = sbrk(size << PAGE_SHIFT);
    assert(ptr != (void *) -1);
    return ptr;
}

inline static void SystemFree(void *ptr) {
	
}

static void*& _next(void *node) {
    // 指针的类型是一个指针，解引用后是一个指针，取前4/8个字节
    return  *(void**)node;
}

class FreeList {
public:
    FreeList() : _size(0), _head(nullptr), _MaxSize(0) {

    }
    // 头插
    void push_front(void *node) {
        assert(node);
        _next(node) = _head;
        _head = node;
        _size++;
    }
    void push_range(void *start, void *end, int n) {
        _next(end) = _head;
        _head = start;
        _size += n;
    }
    // 获取头节点
    void* front() {
        assert(_head);
        return _head;
    }
    // 删除头节点
    void pop_front() {
        assert(_head);
        _head = _next(_head);
        _size--;
    }
    // 取出前n个
    void pop_range(void *&start, void *&end, size_t n) {
        assert(n <= _size);
        start = end = _head;
        for (int i = 0; i < n - 1; i ++) {
            end = _next(end);
        }
        _head = _next(end);
        _next(end) = nullptr;
        _size -= n;
    }
    bool empty() {
        return _size == 0;
    }
    size_t size() {
        return this->_size;
    }
    size_t maxSize() {
        return _MaxSize;
    }
    void add_maxSize(size_t x) {
        _MaxSize += x;
    }
private:
    void *_head;
    size_t _size;
    size_t _MaxSize;
};

// 计算对象大小的对齐规则
//计算对象大小的对其规则
class SizeClass
{
	//分段内存对齐
	// 整体控制在最多10%左右的内碎片浪费
    // [1,128] 8byte对齐       freelist[0,16)
    // [128+1,1024] 16byte对齐   freelist[16,72)
    // [1024+1,8*1024] 128byte对齐   freelist[72,128)
    // [8*1024+1,64*1024] 1024byte对齐     freelist[128,184)
    // [64*1024+1,256*1024] 8*1024byte对齐   freelist[184,208)

	/*size_t _RountUp(size_t size,size_t alignNum)
	{
		size_t alignSize;
		if (size % alignNum != 0)
		{
			alignSize = (size / alignNum + 1) * alignNum; 
		}
		else {
			alignSize = size;
		}
	}*/
public:
	static inline size_t _RountUp(size_t bytes, size_t alignNum)
	{
		return ((bytes + alignNum - 1) & ~(alignNum - 1));//获得的一定是一个>=alignNum且是alignNum的倍数
	}

	static inline  size_t RountUp(size_t size)
	{
		if (size <= 128)
		{
			return _RountUp(size, 8);
		}
		else if (size <= 1024)
		{
			return _RountUp(size, 16);
		}
		else if (size <= 8 * 1024)
		{
			return _RountUp(size, 128);
		}
		else if (size<=64 * 1024)
		{
			return _RountUp(size, 1024);
		}
		else if (size <= 256 * 1024)
		{
			return _RountUp(size, 8 * 1024);
		}
		else
		{
			return _RountUp(size, 1 << PAGE_SHIFT);
		}
	}
	//thread cache一次从中心缓存获取多少个
	static size_t NumMoveSize(size_t size)
	{
		assert(size > 0);
		if (size == 0)return 0;

		//[2,512],一次批量移动多少个对象的（慢启动）上限值
		//小对象给多点
		//大对象给少点
		int num = MAX_BYTES / size;
		if (num < 2)//控制下限
		{
			num = 2;
		}
		if (num > 512)//控制上限 
		{
			num = 512;
		}
		return num;
	}

	//计算central cache一次向系统获取几个页
	//单个对象 8byte
	//...
	//单个对象 256kb
	static size_t NumMovePage(size_t size)
	{
		size_t num = NumMoveSize(size);//小对象的数量
		size_t npage = num * size;//字节数
		npage >>= PAGE_SHIFT;//除以8KB，计算有多少页
		if (npage == 0)//不够补一
			npage = 1;
		return npage;
	}
	
	static inline size_t _Index(size_t bytes, size_t align_shift)
	{
		return ((bytes + (1 << align_shift) - 1) >> align_shift) - 1;//(bytes/align_shift) 向上取整再-1
	}
	//定位自由链表桶在哈希表的位置
	static inline size_t Index(size_t bytes)
	{
		assert(bytes <= MAX_BYTES);

		//每个区间有多少个链
		static int group_array[4] = { 16,56,56,56 };
		if (bytes <= 128)
		{
			return _Index(bytes, 3);
		}
		else if (bytes <= 1024)
		{
			return _Index(bytes - 128, 4) + group_array[0];//还要加上前面不同对齐数桶的偏移量
		}
		else if (bytes <= 8 * 1024)
		{
			return _Index(bytes - 1024, 7) + group_array[0]+group_array[1];
		}
		else if (bytes <= 64 * 1024)
		{
			return _Index(bytes - 1024*8, 10) + group_array[0] + group_array[1]+group_array[2];
		}
		else if (bytes <= 256 * 1024)
		{
			return _Index(bytes - 1024 * 64, 13) + group_array[0] + group_array[1] + group_array[2]+group_array[3];
		}
		else {
			assert(false);
		}
		return 0;
	}

};
// 管理多个连续页大块内存内存跨度结构
// Central每个桶中链表中的节点
struct Span {
    PAGE_ID _PageId = 0; // 大块内存起始的页号
    size_t n = 0;   // 页的数量
    Span* next = nullptr;
    Span* prev = nullptr;
    size_t objSize = 0; //切好的小对象的大小 ，同一个span内的小对象大小是一样的
	size_t useCount = 0; //切好的小块内存，被分配给thread cache的计数， 表示有多少个小块内存被使用
	void *freeList = nullptr; //切好的小块内存的自由链表
	bool _isUse = false;//是否在被使用，防止page cache合并了正在使用的span
};

// Central 每个哈希桶，每个桶其实是一个链表
class SpanList {
public:
    SpanList() {
        head = new Span;
        head->next = head->prev = head;
    }
    void insert(Span *pos, Span *nSpan) {
        assert(pos);
        assert(nSpan);
        nSpan->prev = pos->prev;
        nSpan->next = pos;
        pos->prev->next = nSpan;
        pos->prev = nSpan;
    }
    void erase(Span *span) {
        assert(span);
        assert(span != head);
        span->prev->next = span->next;
        span->next->prev = span->prev;
    }
    void push_front(Span *span) {
        insert(begin(), span);
    }
    Span* begin() {
        return head->next;
    }
    Span* end() {
        return head;
    }
    void pop_front() {
        erase(head->next);
    }
    bool empty() {
        return head->next == head;
    }
    void lock() {
        _mutex.lock();
    }
    void unlock() {
        _mutex.unlock();
    }
private:
    // 带头循环双链表
    Span* head;
    std::mutex _mutex; // 桶锁
};


