//
// Created by czx on 18-12-30.
//
#include "../include/forward_server.h"
#define ListenQueue 200

#define LOCAL_PORT 7001
#define SERVER_PORT 7002
#define SERVER_IP "127.0.0.1"
#define MAX_CONNECT 30

int main() {
    signal(SIGPIPE, SIG_IGN);

    forward_server::threadPool.pool_init(MAX_CONNECT*4);
    forward_server::forward_pool_int(MAX_CONNECT);
    int conn;
    int ss = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in local_sockaddr;
    local_sockaddr.sin_family = AF_INET;
    local_sockaddr.sin_port = htons(LOCAL_PORT);
    local_sockaddr.sin_addr.s_addr =htonl(INADDR_ANY);// inet_addr("192.168.123.227");  ///服务器ip //htonl(INADDR_ANY);
    if(bind(ss, (struct sockaddr* ) &local_sockaddr, sizeof(local_sockaddr))==-1) {
        perror("bind");
        exit(1);
    }
    if(listen(ss, ListenQueue) == -1) {
        perror("listen");
        exit(1);
    }

    struct sockaddr_in client_addr;
    socklen_t length = sizeof(client_addr);
    while(1) {
        printf("waiting for connet!\n");
        conn = accept(ss, (struct sockaddr *) &client_addr, &length);
        if (conn < 0) {
            perror("connect");
            continue;
        }
#if 1
        struct timeval timeout = {1, 0};//3s
        int ret = setsockopt(conn, SOL_SOCKET, SO_SNDTIMEO,  &timeout, sizeof(timeout));
        if (ret < 0) {
            perror("setsockopt SO_SNDTIMEO");
            exit(1);
        }
        ret = setsockopt(conn, SOL_SOCKET, SO_RCVTIMEO,  &timeout, sizeof(timeout));
        if (ret < 0) {
            perror("setsockopt SO_RCVTIMEO");
            exit(1);
        }

        socklen_t optlen = sizeof(struct timeval);

        struct timeval tv;

        memset(&tv,0,sizeof(tv));

        getsockopt(conn, SOL_SOCKET,SO_RCVTIMEO, &tv, &optlen);
#endif

        forward_server *forward = forward_server::forward_pool_get();
        if(forward!=NULL) {
            forward->init(conn,SERVER_IP,SERVER_PORT);
            printf("new connect id=%d \n",forward->id);
        } else
        {
            close(conn);
        }
    }
    close(ss);
    forward_server::threadPool.pool_destroy();
    forward_server::forward_pool_destroy();

    return 0;
}
