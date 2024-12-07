#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <dirent.h>
#include <ctype.h>
#include <pthread.h>
#include <strings.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>

// 初始化监听的套接字
int initListenFd(unsigned short port);

// 启动epoll
void epollRun(int lfd);

// 和客户端建立连接
void* acceptClient(void *arg);

// 接收http请求
void* recvHttpRequest(void *arg);

// 解析http请求
int parseRequestLine(const char *line, int cfd);

// 文件发送给客户端 
void sendFile(const char *fileName, int cfd);

// 发送响应头(状态行和响应头)
void sendHeadMsg(int cfd, int status, const char *descr, const char *type, int length);

// 根据文件后缀找对应的content—type
const char* getFileType(const char *name);

// 将目录发送客户端
void sendDir(const char *dirName, int cfd);

int hexToDec(char c);
// 将被转码的特殊字符恢复原状
void decodeMsg(char * to, char * from);

