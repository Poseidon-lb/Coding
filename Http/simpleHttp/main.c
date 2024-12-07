#include "server.h"

int main(int argc, char * argv[]) {

    if (argc < 3) {
        printf("./a port path\n");
        return -1;
    }

    unsigned short port = atoi(argv[1]);

    // 切换服务器进程工作路径到指定资源路径
    chdir(argv[2]);
    
    // 初始化监听的套接字
    int lfd = initListenFd(port);
    
    // 启动服务器程序    
    epollRun(lfd);

    return 0;
}