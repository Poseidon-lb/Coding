#pragma once
#define _GNU_SOURCE
#include "Channel.hpp"
#include "EventLoop.hpp"
#include <strings.h>
#include <string>

using namespace std;

class EventLoop;
class Dispatcher {
public:
    Dispatcher(EventLoop* evLoop);
    // 添加
    virtual int add();
    // 删除
    virtual int remove();
    // 修改
    virtual int modify();
    // 监测事件
    virtual int dispatch(int timeout = 2);  
    // 虚析构
    virtual ~Dispatcher();
    inline void setChannel(Channel* channel) {
        m_channel = channel;
    }
protected:
    string name = string();
    Channel* m_channel;
    EventLoop* m_evLoop;
};