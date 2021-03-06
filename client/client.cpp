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
#include <stdio.h>
#include <string>
#include <algorithm>
#include <iostream>
#define MYPORT  7001
#define BUFFER_SIZE 1024

#define STOP_SSR  0x11
#define STAR_SSR  0x22
char* getCmdOption(char ** begin, char ** end, const std::string & option)
{
    char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

int cmdParse(int argc, char * argv[], unsigned char &toDo)
{
    char * toDo_c = getCmdOption(argv, argv + argc, "-t");
    std::string toDo_s;
    if (toDo_c) {
        toDo_s = std::string(toDo_c);
        printf("todo=%s\n",toDo_s.c_str());
        if(toDo_s.compare("start")==0) {
            toDo = STAR_SSR;
            printf("toDo=STAR_SSR\n");
        }
        else if(toDo_s.compare("stop")==0)
        {
            toDo=STOP_SSR;
            printf("toDo=STOP_SSR\n");
        } else
        {
            toDo=0;
            printf("toDo=0\n");
        }
    }


    if (!toDo_c )
    {
        std::cout << "Usage: ./app_name "
                  << "-t start "
                  << "-t stop " << std::endl;
        return -1;
    }

    return 0;
}
int main(int argc, char** argv)
{
    unsigned char toDo=0;
    cmdParse(argc, argv, toDo);
    ///定义sockfd
    int sock_cli = socket(AF_INET,SOCK_STREAM, 0);

    ///定义sockaddr_in
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(MYPORT);  ///服务器端口
    servaddr.sin_addr.s_addr = inet_addr("3.16.147.254");  ///服务器ip

    //连接服务器，成功返回0，错误返回-1
    if (connect(sock_cli, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("connect");
        exit(1);
    }
    printf("connect sucess!\n");

    int ret=send(sock_cli,&toDo,sizeof(toDo),0);
    printf("send comand ret=%d\n",ret);
    close(sock_cli);
    return 0;
}
