#pragma once
#define _GNU_SOURCE
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>
#include "Buffer.hpp"
#include "Channel.hpp"
#include "EventLoop.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

//#define MSG_SEND_AUTO

struct TcpConnection {
    struct Channel *channel;
    struct EventLoop *evLoop;
    struct Buffer *readBuff, *writeBuffer;
    struct HttpRequest *request;
    struct HttpResponse *response;
    char name[32];
};

// 初始化
struct TcpConnection* tcpConnectionInit(int fd, struct EventLoop *evLoop);
// 释放资源
void tcpConnectionDestroy(struct TcpConnection* conn);