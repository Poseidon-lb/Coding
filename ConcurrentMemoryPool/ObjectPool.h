#pragma once

#include "Common.h"

template <class T> 
class ObjectPool {
public:
    T* New() {
        T *object = nullptr;
        if (freeList != nullptr) {
            object = static_cast<T*>(freeList);
            freeList = _next(freeList);
        } else {
            if (leftBytes < sizeof (T)) {
                leftBytes = 128 * 1024;
                memory = static_cast<char *>(SystemAlloc(leftBytes >> 13));
                if (memory == nullptr) {
                    std::throw bad_alloc();
                }
            } 
            obj = static_cast<T*>(memory);
            // T的大小可能没一个指针大
            size_t objSize = std::max(sizeof (void*), sizeof (T));
            memory += objSize;
            leftBytes -= objSize;
        }
        // 定位new，显示调用T的构造函数初始化
        //定位new是C++的一个高级特性，允许在已分配的内存上构造对象，常用于内存池或自定义内存管理
		new(obj)T;

        return obj;
    }
    
    void Delete(T *obj) {
        // 使用定位new调用构造函数必须显示调用析构函数
        obj->~T();
        _next(obj) = freeList;
        freeList = obj;
    }
    
private:
    void *freeList = nullptr;
    size_t leftBytes = 0; //内存块中剩余的字节数;
    char* memory = nullptr;//指向内存块的指针
};