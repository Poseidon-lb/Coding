#pragma once
#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/uio.h>
#include <string.h>
#include <strings.h>
#include <arpa/inet.h>

struct Buffer {
    // 指向内存的指针
    char *data;
    int capacity;
    int readPos;
    int writePos;
};

// 初始化buffer
struct Buffer* bufferInit(int size);
void bufferDestroy(struct Buffer *buffer);
// 剩余可读大小
int bufferReadableSize(struct Buffer *buffer);
// 剩余可写大小
int bufferWriteableSize(struct Buffer *buffer);
// 扩容
void bufferExtendRoom(struct Buffer *buffer, int size);
// 写内存：1.直接写 
int bufferAppendData(struct Buffer *buffer, const char *data, int size);
int bufferAppendString(struct Buffer *buffer, const char *data);
// 2.接收套接字通信
int bufferSocketRead(struct Buffer *buffer, int fd);
// 根绝\r\n取出一行，找到其在数据中的位置，返回该位置
char* bufferFindCRLF(struct Buffer *buffer);
// 发送数据
int bufferSendData(struct Buffer *buffer, int socket);