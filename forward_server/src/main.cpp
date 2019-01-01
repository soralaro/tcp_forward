//
// Created by czx on 18-12-30.
//
#include "../include/forward_server.h"
#define PORT 7001
#define ListenQueue 200
int DGSafeActivation(std::string& activation_code)
{
    int ret = 0;
#if SAFENET
    ret =DG_Safe_Set_Active_Code(activation_code.data());
    if(ret != 0)
    {
        return -1;
    }

    ret = DG_Safe_Init();
    if(ret != 0)
    {
        ret = DG_Safe_Activation(activation_code.data());

        if(ret != 0)
        {
            return -1;
        }
        ret = DG_Safe_Init();
        if(ret != 0)
        {
            return -1;
        }
    }
#endif
    return ret;
}
int main() {
    signal(SIGPIPE, SIG_IGN);
    std::string ActivationCode="6d3fe3b7-36d7-4382-bf7b-e6b8bfe639cc";
    int result = DGSafeActivation(ActivationCode);

    std::cout <<"DGSafeActivation result=" << result << std::endl;

    forward_server::threadPool.pool_init(400);
    forward_server::forward_pool_int(100);
    int conn;
    int ss = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_sockaddr;
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(PORT);
    //printf("%d\n",INADDR_ANY);
    server_sockaddr.sin_addr.s_addr =htonl(INADDR_ANY);// inet_addr("192.168.123.227");  ///服务器ip //htonl(INADDR_ANY);
    if(bind(ss, (struct sockaddr* ) &server_sockaddr, sizeof(server_sockaddr))==-1) {
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
        ///成功返回非负描述字，出错返回-1
        printf("waiting for connet!\n");
        conn = accept(ss, (struct sockaddr *) &client_addr, &length);
        if (conn < 0) {
            perror("connect");
            //exit(1);
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
            forward->init(conn);
            printf("%s\n", forward->server_ip.c_str());
            printf("id=%d clien tv.tv_sec=%ld,tv_usec=%ld \n",forward->id,tv.tv_sec,tv.tv_usec);
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
