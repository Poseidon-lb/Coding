#pragma once
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdbool.h>
#include <strings.h>

// 定义函数指针
typedef void* (*handleFunc)(void* arg);

// 定义文件描述符的读写事件
enum FdEvent {
    ReadEvent = 0x01,
    WriteEvent = 0x02
};

struct Channel {
    int fd;                     // 文件描述符
    int events;                 // 检测事件
    handleFunc readCallBack;    // 读回调
    handleFunc writeCallBack;   // 写回调
    void *arg;                  // 回调函数参数
};

// 初始化一个Channel
struct Channel* ChannelInit(int fd, int events, handleFunc readFunc, handleFunc writeFunc, void* arg);

// 修改fd的写事件(检测 or 不检测)
void writeEventEnable(struct Channel* channel, bool flag);

// 判断是否需要检测对应描述符的写事件
bool isWriteEventEntable(struct Channel * channel);
