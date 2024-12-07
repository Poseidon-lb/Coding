#include "TcpConnection.h"
#include <strings.h>
#include "Log.h"

void* processRead(void *arg) {
    Debug("开始读取客户端发送的信息");
    struct TcpConnection *conn = (struct TcpConnection *)arg;
    // 接收数据
    int count = bufferSocketRead(conn->readBuff, conn->channel->fd);
    Debug("客户端的信息接收完成");
    if (count > 0) {
#ifdef MSG_SEND_AUTO
        writeEventEnable(conn->channel, true);
        eventLoopAddTask(conn->evLoop, conn->channel, MODIFY);
#endif
        // 接收到了数据，接收Http请求，解析Http请求
        bool ok = parseHttpRequest(conn->request, conn->readBuff, 
            conn->response, conn->writeBuffer, conn->channel->fd);
        Debug("成功回复客户端数据");
        if (!ok) {
            char *errMsg = "HTTP/1.1 400 Bad Request\r\n\r\n";
            bufferAppendString(conn->writeBuffer, errMsg);
        }
    }
#ifndef MSG_SEND_AUTO
    eventLoopAddTask(conn->evLoop, conn->channel, DELETE);
#endif
    return NULL;
}

void* processWrite(void *arg) {
    struct TcpConnection *conn = (struct TcpConnection *)arg;
    // 发送数据
    int count = bufferSendData(conn->writeBuffer, conn->channel->fd);
    if (count > 0) {
        // 数据全部被发送完
        if (bufferReadableSize(conn->writeBuffer) == 0) {
            // // 不再检测写事件 -- 修改channel中检测的事件
            // writeEventEnable(conn->channel, false);
            // // 修改dispatcher检测的集合
            // eventLoopAddTask(conn->evLoop, conn->channel, MODIFY);
            // 断开连接
            eventLoopAddTask(conn->evLoop, conn->channel, DELETE);
        }
    }
    return NULL;
}

struct TcpConnection* tcpConnectionInit(int fd, struct EventLoop *evLoop) {
    struct TcpConnection* conn = (struct TcpConnection *)malloc(sizeof (struct TcpConnection));
    conn->evLoop = evLoop;
    conn->readBuff = bufferInit(10240);
    conn->writeBuffer = bufferInit(10240);
    sprintf(conn->name, "Connection-%d", fd);
    conn->request = httpRequesInit();
    conn->response = httpResponseInit();
    struct Channel *channel = ChannelInit(fd, ReadEvent, processRead, processWrite, conn);
    conn->channel = channel;
    Debug("conn初始化完成");
    eventLoopAddTask(evLoop, channel, ADD);
    return conn;
}

void tcpConnectionDestroy(struct TcpConnection* conn) {
    if (conn == NULL) {
        return;
    }
    if (conn->readBuff && bufferReadableSize(conn->readBuff) == 0 &&
        conn->writeBuffer && bufferReadableSize(conn->writeBuffer) == 0) {
            destroyChannel(conn->evLoop, conn->channel);
            bufferDestroy(conn->readBuff);
            bufferDestroy(conn->writeBuffer);
            httpRequestDestroy(conn->request);
            httpResponseDestroy(conn->response);
        }
    free(conn);
}