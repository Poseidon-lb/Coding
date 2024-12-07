#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "TcpServer.h"
#include "Log.h"

int main(int argc, char * argv[]) {
    if (argc < 3) {
        printf("./a port path\n");
        return -1;
    }

    Debug("进入main了");

    unsigned short port = atoi(argv[1]);

    // 切换服务器进程工作路径到指定资源路径
    chdir(argv[2]);

    struct TcpServer* server = tcpServerInit(port, 4);
    tcpServerRun(server);
    return 0;
}