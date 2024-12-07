#define _GNU_SOURCE
#include "HttpRequest.h"
#include <dirent.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>

#define HeaderSize 12

void httpRequestReset(struct HttpRequest* request) {
    request->CurState = ParseReqLine;
    request->method = NULL;
    request->url = NULL;
    request->version = NULL;
    request->reqHeadersNum = 0;
}

void httpRequestResetEx(struct HttpRequest* request) {
    if (request->method != NULL) {
        free(request->method);
    }
    if (request->url != NULL) {
        free(request->url);
    }
    if (request->version != NULL) {
        free(request->version);
    }
    if (request->reqHeaders != NULL) {
        for (int i = 0; i < HeaderSize; i ++) {
            if (request->reqHeaders[i].key != NULL) {
                free(request->reqHeaders[i].key);
            }
            if (request->reqHeaders[i].value != NULL) {
                free(request->reqHeaders[i].value);
            }
        }
        free(request->reqHeaders);
    }
    httpRequestReset(request);
}

struct HttpRequest* httpRequesInit() {
    struct HttpRequest *request = (struct HttpRequest*)malloc(sizeof (struct HttpRequest));
    httpRequestReset(request);
    request->reqHeaders = (struct RequestHeader *)malloc(sizeof (struct RequestHeader) * HeaderSize);
    return request;
}

void httpRequestDestroy(struct HttpRequest* request) {
    if (request == NULL) {
        return;
    }
    httpRequestResetEx(request);
    free(request);
}

enum HttpRequestState httpRequestState(struct HttpRequest* request) {
    return request->CurState;
}

void httpRequestAddHeader(struct HttpRequest* request, const char *key, const char *value) {
    request->reqHeaders[request->reqHeadersNum].key = (char *)key;
    request->reqHeaders[request->reqHeadersNum++].value = (char *)value;
}

char* httpRequestGetHeader(struct HttpRequest* request, const char *key) {
    if (request == NULL) {
        return NULL;
    }
    for (int i = 0; i < request->reqHeadersNum; i ++) {
        if (strncasecmp(request->reqHeaders[i].key, key, strlen(key)) == 0) {
            return request->reqHeaders[i].value;
        }
    }
    return NULL;
}

char* splitRequestLine(const char *start, const char *end, const char *sub, char **ptr) {
    char *space = end;
    if (sub != NULL) {
        space = memmem(start, end - start, sub, strlen(sub));
        assert(space != NULL);
    }
    int length = space - start;
    *ptr = (char *)malloc(length + 1);
    strncpy(*ptr, start, length);
    (*ptr)[length] = '\0';
    return space + 1;
}

bool parseHttpRequestLine(struct HttpRequest* request, struct Buffer *readBuf) {
    // 读出请求行, 保存字符串结束地址
    char *end = bufferFindCRLF(readBuf);
    // 保存字符串起始地址
    char *start = readBuf->data + readBuf->readPos;
    int lineSize = end - start;
    Debug("请求行的长度= %d", lineSize);
    if (lineSize <= 0) {
        return false;
    }
    // get /xxx/xxx.txt http/1.1
    // 请求方式
    start = splitRequestLine(start, end, " ", &request->method);
    Debug("请求方式解析完毕");
    // 请求资源
    start = splitRequestLine(start, end, " ", &request->url);
    Debug("请求资源解析完毕");
    // 获取http版本
    splitRequestLine(start, end, NULL, &request->version);
    Debug("请求版本解析完毕");
    // 更新buffer
    readBuf->readPos += lineSize + 2;
    // 修改状态
    request->CurState = ParseReqHeaders;
    
    return true;
}

bool parseHttpRequestHeader(struct HttpRequest* request, struct Buffer *readBuf) {
    char *end = bufferFindCRLF(readBuf);
    if (end == NULL) {
        return false;
    }
    char *start = readBuf->data + readBuf->readPos;
    int lineSize = end - start;
    char *middle = memmem(start, lineSize, ": ", 2);
    if (middle == NULL) {
        // 解析完所有请求头
        readBuf->readPos += 2;
        request->CurState = ParseReqDone;
        return true;
    }
    int keySize = middle - start;
    char *key = (char *)malloc(keySize + 1);
    strncpy(key, start, keySize);
    key[keySize] = '\0';

    int valueSize = end - middle - 2;
    char *value = (char *)malloc(valueSize + 1);
    strncpy(value, middle + 2, valueSize);
    value[valueSize] = '\0';

    httpRequestAddHeader(request, key, value);
    readBuf->readPos += lineSize + 2;
    
    return true;
}

int hexToDec(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    } 
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
}

void decodeMsg(char *to, char *from) {
    for (; *from != '\0'; to++, from ++) {
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2])) {
            *to = hexToDec(from[1]) * 16 + hexToDec(from[2]);
            from += 2;
        } else {
            *to = *from;
        }
    }
    *to = '\0';
}

const char* getFileType(const char *name) {
    // a.jpg a.mp4 a.html
    // 自右向左查找‘.’字符, 如不存在返回NULL
    const char* dot = strrchr(name, '.');
    if (dot == NULL)
        return "text/plain; charset=utf-8";	// 纯文本
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp(dot, ".wav") == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";

    return "text/plain; charset=utf-8";
}

void sendFile(const char *fileName, struct Buffer *sendBuf, int cfd) {
    // 读一部分发一部分。
    // 采用的tcp协议，双方会建立连接, tcp是流式传输协议,会发送完
    
    // 打开文件
    int fd = open(fileName, O_RDONLY);
    assert(fd > 0);

#if 1
    while (true) {
        char buf[1024]; // 不读取太大是因为send有缓存区
        int len = read(fd, buf, sizeof buf);
        if (len > 0) {
            //send(cfd, buf, len, 0);
            bufferAppendData(sendBuf, buf, len);
#ifndef MSG_SEND_AUTO
    bufferSendData(sendBuf, cfd);
#endif
            //usleep(10);     
        } else if (len == 0) {
             break;
        } else {
            perror("read");
        }
    }
#else
    struct stat st;
    stat(fileName, &st);
    //sendfile(cfd, fd, NULL, st.st_size); // 号称零拷贝
    
    // sendfile 也是有缓存区的，当文件过大，一次就发送不了，需要循环发送
    off_t offset = 0;
    int size = st.st_size;
    while (offset < size) {
        sendfile(cfd, fd, &offset, size - offset);
    }
#endif
    close(fd);
}

/*
<html>
    <head>
        <title>test</title>
    </head>
    <body>
        <table>
            <tr>
                <td></td>
                <td></td>
            </tr>
            <tr>
                <td></td>
                <td></td>
            </tr>
        </table>
    </body>
</html>
*/

void sendDir(const char *dirName, struct Buffer *sendBuf, int cfd) {
    char buf[4096] = {0};
    sprintf(buf, "<html><head><title>%s</title></head><body><table>", dirName);

    // namelist 指向 struct dirent* tmp[]
    struct dirent **namelist;
    int cnt = scandir(dirName, &namelist, NULL, alphasort);
    for (int i = 0; i < cnt; i ++) {
        char *name = namelist[i]->d_name;
        char subPath[1024] = {0};
        sprintf(subPath, "%s/%s", dirName, name);
        struct stat st;
        stat(subPath, &st);
        if (S_ISDIR(st.st_mode)) {
            // a标签实现html网页跳转 ，<a href="">name</a>  
            // 当前是name,可以让当前的网页跳转到网页herf上去
            sprintf(buf + strlen(buf), 
            "<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>",
            name, name, st.st_size);
        } else {
            sprintf(buf + strlen(buf), 
            "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
            name, name, st.st_size);
        }
        //send(cfd, buf, strlen(buf), 0);
        bufferAppendString(sendBuf, buf);
#ifndef MSG_SEND_AUTO
    bufferSendData(sendBuf, cfd);
#endif
        memset(buf, 0, sizeof buf);
        free(namelist[i]);
    } 
    sprintf(buf, "</table></body></html>");
    //send(cfd, buf, strlen(buf), 0);
    bufferAppendString(sendBuf, buf);
#ifndef MSG_SEND_AUTO
    bufferSendData(sendBuf, cfd);
#endif
    free(namelist);
}

// 处理基于get的http请求
bool processHttpRequest(struct HttpRequest* request, struct HttpResponse* response) {
    if (strcasecmp(request->method, "get") != 0) {
        return false;
    }
    decodeMsg(request->url, request->url);
    // 处理客户端请求的静态资源(目录或者文件)
    char * file = NULL;
    if (strcmp(request->url, "/") == 0) {
        file = "./";
    } else {
        file = request->url + 1;
    }
    // 获取文件属性
    struct stat st;
    int ret = stat(file, &st);
    if (ret == -1) {
        // 文件不存在 - 404
        //sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);
        //sendFile("404.html", cfd);
        strcpy(response->fileName, "404.html");
        response->statusCode = NotFound;
        strcpy(response->statusMsg, "Not Found");
        httpResponseAddHeader(response, "Content-type", getFileType(".html"));
        response->sendDataFunc = sendFile;
        return 0;
    }

    strcpy(response->fileName, file);
    response->statusCode = OK;
    strcpy(response->statusMsg, "OK");
    // 判断文件类型
    if (S_ISDIR(st.st_mode)) {
        // 如果是目录，把目录的内容发送给客户端
        //sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
        //sendDir(file, cfd);
        httpResponseAddHeader(response, "Content-type", getFileType(".html"));
        response->sendDataFunc = sendDir;
    } else {
        // 把文件的内容发送给客户端
        //sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size);
        //sendFile(file, cfd);
        char tmp[12] = {0};
        sprintf(tmp, "%ld", st.st_size);
        httpResponseAddHeader(response, "Content-length", tmp);
        httpResponseAddHeader(response, "Content-type", getFileType(file));
        response->sendDataFunc = sendFile;
    }
    return true;
}

bool parseHttpRequest(struct HttpRequest* request, struct Buffer *readBuf,
    struct HttpResponse* response, struct Buffer *sendBuf, int socket) {
    while (request->CurState != ParseReqDone) {
        if (request->CurState == ParseReqLine) {
            if (!parseHttpRequestLine(request, readBuf)) {
                return false;
            }
            Debug("请求行解析完成");
        }
        if (request->CurState == ParseReqHeaders) {
            if (!parseHttpRequestHeader(request, readBuf)) {
                return false;
            }
        }
        if (request->CurState == ParseReqBody) {
            
        }
    }
    Debug("客户端信息全部解析完成");
    // 解析完毕，准备回复的数据
    // 根据解析出的原始数据，对对客户的请求做出处理
    processHttpRequest(request, response);
    // 组织响应数据并发送给客户端
    httpResponsePrepareMsg(response, sendBuf, socket);
    request->CurState = ParseReqLine;
    return true;
}

