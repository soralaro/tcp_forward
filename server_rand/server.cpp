//
// Created by czx on 18-12-16.
//
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <thread>
#include <iostream>
#define PORT 7001
#define QUEUE 20
#define STOP_SSR  0x11
#define STAR_SSR  0x22
int conn;
void thread_task() {
}

void commandPro(unsigned char command)
{
    switch(command)
    {
        case STAR_SSR:
            system("/home/ubuntu/ssr_star_com.sh");
            break;
        case STOP_SSR:
            system("/home/ubuntu/ssr_stop_com.sh");
            break;
        default:
            break;
    }
}
int main() {
    int ss = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_sockaddr;
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(PORT);
    //printf("%d\n",INADDR_ANY);
    server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(ss, (struct sockaddr* ) &server_sockaddr, sizeof(server_sockaddr))==-1) {
        perror("bind");
        exit(1);
    }
    if(listen(ss, QUEUE) == -1) {
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
        struct timeval timeout={3,0};//3s
        int ret=setsockopt(conn,SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof(timeout));
        if(ret<0)
        {
            perror("setsockopt SO_SNDTIMEO");
            exit(1);
        }
        ret=setsockopt(conn,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));
        if(ret<0)
        {
            perror("setsockopt SO_RCVTIMEO");
            exit(1);
        }
        printf("connect suc\n");
        char buffer[1024*1024];
        //主线程
        while (1) {

            memset(buffer, 0, sizeof(buffer));
            int len = recv(conn, buffer, sizeof(buffer), 0);
            if(len>0) {
                printf("recv len%d\n", len);
            }
            else if(len==0)
            {
                printf("recv time out\n");
                break;
            }
            else
            {
                printf("recv erro \n");
                break;
            }
            for(int i=0;i<1024*1024;i++)
            {
                buffer[i]=rand();
            }
            int ret = send(conn, buffer, sizeof(buffer), 0);
            printf("send ret=%d\n", ret);
            if(ret<=0)
            {
                break;
            }
            sleep(1);
        }
        close(conn);
    }
    close(ss);
    return 0;
}
