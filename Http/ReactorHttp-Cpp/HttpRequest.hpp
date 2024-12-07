#pragma once
#define _GNU_SOURCE
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include "Buffer.h"
#include <sys/stat.h>
#include <unistd.h>
#include "HttpResponse.h"
#include <ctype.h>
#include "Log.h"

struct RequestHeader {
    char *key;
    char *value;
};

enum HttpRequestState {
    ParseReqLine, 
    ParseReqHeaders,
    ParseReqBody,
    ParseReqDone
};

// 定义http请求结构体
struct HttpRequest {
    char *method;
    char *url;
    char *version;
    struct RequestHeader *reqHeaders;
    int reqHeadersNum;
    enum HttpRequestState CurState;
};

// 重置
void httpRequestReset(struct HttpRequest* request);
void httpRequestResetEx(struct HttpRequest* request);
// 初始化
struct HttpRequest* httpRequesInit();
// 销毁
void httpRequestDestroy(struct HttpRequest* request);
// 获取处理状态
enum HttpRequestState httpRequestState(struct HttpRequest* request);
// 添加请求头
void httpRequestAddHeader(struct HttpRequest* request, const char *key, const char *value);
// 根据key获取请求头的value
char* httpRequestGetHeader(struct HttpRequest* request, const char *key);
// 解析请求行
bool parseHttpRequestLine(struct HttpRequest* request, struct Buffer *readBuf);
// 解析请求头中的一行
bool parseHttpRequestHeader(struct HttpRequest* request, struct Buffer *readBuf);
// 解析http请求协议
bool parseHttpRequest(struct HttpRequest* request, struct Buffer *readBuf,
    struct HttpResponse* response, struct Buffer *sendBuf, int socket);
// 处理http请求协议
bool processHttpRequest(struct HttpRequest* request, struct HttpResponse* response);