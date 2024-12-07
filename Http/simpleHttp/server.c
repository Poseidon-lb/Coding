#include "server.h"

struct FdInfo {
    int fd, epfd;
    pthread_t tid;
};

// 初始化监听的套接字
int initListenFd(unsigned short port) {
    // 创建监听的套接字
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    // 设置端口复用, 2MSL(TCP协议)的等待时长,大概一分钟
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
    // 绑定端口
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(lfd, (struct sockaddr *)&addr, sizeof addr);
    // 设置监听
    listen(lfd, 128);
    return lfd;
}

void sendHeadMsg(int cfd, int status, const char *descr, const char *type, int length) {
    // 状态行
    char buf[4096] = {0};
    sprintf(buf, "http/1.1 %d %s\r\n", status, descr);
    //响应头
    sprintf(buf + strlen(buf), "content-type: %s\r\n", type);
    sprintf(buf + strlen(buf), "content-length: %d\r\n\r\n", length);
    send(cfd, buf, strlen(buf), 0);
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

void sendDir(const char *dirName, int cfd) {
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
        send(cfd, buf, strlen(buf), 0);
        memset(buf, 0, sizeof buf);
        free(namelist[i]);
    } 
    sprintf(buf, "</table></body></html>");
    send(cfd, buf, strlen(buf), 0);
    free(namelist);
}

void sendFile(const char *fileName, int cfd) {
    // 读一部分发一部分。
    // 采用的tcp协议，双方会建立连接, tcp是流式传输协议,会发送完
    
    // 打开文件
    int fd = open(fileName, O_RDONLY);
    assert(fd > 0);

#if 0
    while (1) {
        char buf[1024];
        int len = read(fd, buf, sizeof buf);
        if (len > 0) {
            send(cfd, buf, len, 0);
            usleep(10); 
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

void decodeMsg(char * to, char * from) {
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

int parseRequestLine(const char *line, int cfd) {
    // 解析请求行 请求方式 /请求的资源路径 版本
    char method[12]; 
    char path[1024];
    sscanf(line, "%[^ ] %[^ ]", method, path);
    if (strcasecmp(method, "get") != 0) {
        return -1;
    }
    // 将被转码的特殊字符恢复原状
    decodeMsg(path, path);
    // 处理客户端请求的静态资源(目录或者文件)
    char * file = NULL;
    if (strcmp(path, "/") == 0) {
        file = "./";
    } else {
        file = path + 1;
    }
    // 获取文件属性
    struct stat st;
    int ret = stat(file, &st);
    if (ret == -1) {
        // 文件不存在 - 404
        sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);
        sendFile("404.html", cfd);
        return 0;
    }

    // 判断文件类型
    if (S_ISDIR(st.st_mode)) {
        // 如果是目录，把目录的内容发送给客户端
        sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
        sendDir(file, cfd);
    } else {
        // 把文件的内容发送给客户端
        sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size);
        sendFile(file, cfd);
    }
    return 0;
}

void* acceptClient(void *arg) {
    struct FdInfo *info = (struct FdInfo *) arg;
    int lfd = info->fd;
    int epfd = info->epfd;
    
    // 建立连接
    int cfd = accept(lfd, NULL, NULL);

    // epoll工作效率最高的模式是边沿非阻塞模式
    // 设置非阻塞
    int flag = fcntl(cfd, F_GETFL);             // 得到原本属性
    fcntl(cfd, F_SETFL, flag | O_NONBLOCK);     // 追加非阻塞属性
    
    //  将文件描述符添加到epoll模型中
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;  // 设置边沿属性
    ev.data.fd = cfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
    free(info);
    return NULL;
}

void* recvHttpRequest(void *arg) {
    struct FdInfo *info = (struct FdInfo *) arg;
    int cfd = info->fd;
    int epfd = info->epfd;
    int len = 0, totle = 0;
    char buf[4096] = {0};   // 接收到本地

    while ((len = recv(cfd, buf + totle, sizeof (buf) - totle, 0)) > 0) {
        totle += len;
    }

    // 数据读取完毕.(数据读取完和数据读取失败返回值都是-1,借助errno判断)
    if (len == -1 && errno == EAGAIN) {
        // get请求主要是解析请求行，其他不重要
        // 解析请求行
        char *pt = strstr(buf, "\r\n");
        *pt = '\0';
        parseRequestLine(buf, cfd);
    } else if (len == 0) {
        // 客户端断开连接
        epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
        close(cfd);
    }
    free(info);
    return NULL;
}

void epollRun(int lfd) {
    int epfd = epoll_create(8);

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = lfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
    struct epoll_event evs[1024];

    int sz = sizeof (evs) / sizeof (struct epoll_event);
    while (1) {
        int cnt = epoll_wait(epfd, evs, sz, -1);
        for (int i = 0; i < cnt; i ++) {
            int fd = evs[i].data.fd;
            struct FdInfo *info = (struct FdInfo *)malloc(sizeof (struct FdInfo));
            info->epfd = epfd;
            info->fd = fd;
            if (fd == lfd) {
                // 建立新连接
                //acceptClient(epfd, fd);
                pthread_create(&info->tid, NULL, acceptClient, info);
            } else {
                // 接收对端的数据
                //recvHttpRequest(fd, epfd);
                pthread_create(&info->tid, NULL, recvHttpRequest, info);
            }
        }
    }
}
