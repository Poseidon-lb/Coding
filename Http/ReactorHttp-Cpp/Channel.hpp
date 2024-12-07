#pragma once
#define _GNU_SOURCE
#include <stdlib.h>
#include <strings.h>

// 定义函数指针
using handleFunc = void* (*)(void*);

// 定义文件描述符的读写事件
enum class FdEvent {
    ReadEvent = 0x01,
    WriteEvent = 0x02
};

class Channel {
public:
    handleFunc readCallBack;    // 读回调
    handleFunc writeCallBack;   // 写回调  
    // 初始化一个Channel
    Channel(int fd, int events, handleFunc readFunc, handleFunc writeFunc, void* arg);
    // 修改fd的写事件(检测 or 不检测)
    void writeEventEnable(bool flag);
    // 判断是否需要检测对应描述符的写事件
    bool isWriteEventEntable();
    inline int getEvent() {
        return m_events;
    }
    inline int getSocket() {
        return m_fd;
    }
    inline const void* getArg() {
        return m_arg;
    }
private:
    int m_fd;                     // 文件描述符
    int m_events;                 // 检测事件
    void *m_arg;                  // 回调函数参数
};
