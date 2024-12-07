#pragma once
#define _GNU_SOURCE
#include "Channel.h"
#include "EventLoop.h"
#include <strings.h>

struct EventLoop;

struct Dispatcher {
    // 初始化   
    void* (*init)();
    // 添加
    int (*add)(struct Channel* channel, struct EventLoop* evLoop);
    // 删除
    int (*remove)(struct Channel* channel, struct EventLoop* evLoop);
    // 修改
    int (*modify)(struct Channel* channel, struct EventLoop* evLoop);
    // 监测事件
    int (*dispatch)(struct EventLoop* evLoop, int timeout);
    // 清楚数据(关闭fd or 释放内存)
    int (*clear)(struct EventLoop* evLoop);
};