//
// Created by czx on 18-12-30.
//
#include "../include/forward.h"
#include "../include/server.h"
#include <algorithm>
#ifdef _WIN64
#include<winsock2.h>
#include<ws2tcpip.h>
#endif
#define ListenQueue 200

#define LOCAL_PORT 7102
#define SERVER_PORT 7000
#define SERVER_IP "127.0.0.1"
#define MAX_CONNECT 500
#define ENCRYP_KEY  0Xff
#define ENCRYP_KEY_2  0Xff
#define USR_ID      2
#define DES_KEY     "qwertyuiopasdfgh"
#define DES_KEY2    "qwertyuiopasdfgh"
char* getCmdOption(char ** begin, char ** end, const std::string & option)
{
    char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

int cmdParse(int argc, char * argv[], int& local_port, int& server_port, std::string& server_ip,int& max_connect,unsigned char& encryp_key,unsigned char& encryp_key_2,char *des_key,char *des_key_2)
{
    char *stop_str;
    char * local_port_c = getCmdOption(argv, argv + argc, "-l");
    if (local_port_c)
        local_port = (int)strtol(local_port_c,&stop_str,10);

    char * server_port_c = getCmdOption(argv, argv + argc, "-s");
    if (server_port_c)
        server_port = (int)strtol(server_port_c,&stop_str,10);

    char * server_ip_c = getCmdOption(argv, argv + argc, "-i");
    if (server_ip_c)
        server_ip = std::string(server_ip_c);

    char * max_connect_c = getCmdOption(argv, argv + argc, "-n");
    if (max_connect_c)
        max_connect = (int)strtol(max_connect_c,&stop_str,10);

    char * encryp_key_c = getCmdOption(argv, argv + argc, "-h");
    if (encryp_key_c)
        encryp_key =(u_char)strtol(encryp_key_c,&stop_str,16);

    char * encryp_key_2_c = getCmdOption(argv, argv + argc, "-e");
    if (encryp_key_2_c)
        encryp_key_2=(u_char)strtol(encryp_key_2_c,&stop_str,16);

    char * des_key_c = getCmdOption(argv, argv + argc, "-d");
    if (des_key_c)
        memcpy(des_key,des_key_c,16);

    char * des_key_2_c = getCmdOption(argv, argv + argc, "-k");
    if (des_key_2_c)
        memcpy(des_key_2,des_key_2_c,16);

#if 0
    if (argc<2)
    {
        std::cout << "Usage: ./app_name "
                  << "-l local_port "
                  << "-s SERVER_PORT "
                  << "-i server_ip "
                  << "-n MAX_CONNECT "
                  << "-h encryp_key "
                  << "-e encryp_key_2 "
                  << "-d des_key "
                  << "-k des_key_2 "
                  << std::endl;
        return -1;
    }
#endif
    return 0;
}
int main(int argc, char** argv) {
#ifdef _WIN64
    WORD winSocketVersion;
    WSADATA wsaDATA;
    winSocketVersion = MAKEWORD(2,2);

    int return_num;
    //WinSocket 初始化 
    return_num = WSAStartup(winSocketVersion,&wsaDATA);
    if(return_num!=0){
        std::cout<<"WSAStartup() failed！"<<std::endl;
        return 0; 
    }
#else
    signal(SIGPIPE, SIG_IGN);
#endif

    int local_port=LOCAL_PORT;
    int server_port=SERVER_PORT;
    std::string server_ip=SERVER_IP;
    int max_connect=MAX_CONNECT;
    unsigned char encryp_key=ENCRYP_KEY;
    unsigned char encryp_key_2=ENCRYP_KEY_2;
    char *default_des_key =DES_KEY;
    char *default_des_key_2 =DES_KEY2;
    char des_key[17];
    memcpy(des_key,default_des_key,sizeof(des_key));
    char des_key_2[17];
    memcpy(des_key_2,default_des_key_2,sizeof(des_key_2));
    cmdParse(argc, argv, local_port, server_port, server_ip, max_connect, encryp_key, encryp_key_2,des_key,des_key_2);
    forward::setKey(encryp_key);
    ThreadPool::pool_init(max_connect+3);
    forward::forward_pool_int(max_connect);
    server Server;
    server::user_id=USR_ID;
    Server.setKey(encryp_key);
    Server.setKey_2(encryp_key_2);
    Server.setDesKey(des_key);
    Server.setDesKey_2(des_key_2);
    Server.init(server_ip,server_port);

    int conn;
    int ss = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in local_sockaddr;
    local_sockaddr.sin_family = AF_INET;
    local_sockaddr.sin_port = htons(local_port);
    local_sockaddr.sin_addr.s_addr =htonl(INADDR_ANY);//inet_addr("127.0.0.1");//htonl(INADDR_ANY);
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
        //printf("waiting for connet!\n");
        conn = accept(ss, (struct sockaddr *) &client_addr, &length);
        if (conn < 0) {
            perror("connect");
            continue;
        }
#ifdef _WIN64
        int timeout=6000;
        int ret = setsockopt(conn, SOL_SOCKET, SO_SNDTIMEO,(char *)&timeout, sizeof(timeout));
        if (ret < 0) {
            perror("setsockopt SO_SNDTIMEO");
            exit(1);
        }
        ret = setsockopt(conn, SOL_SOCKET, SO_RCVTIMEO,(char *)&timeout, sizeof(timeout));
        if (ret < 0) {
            perror("setsockopt SO_RCVTIMEO");
            exit(1);
        }
#else
        struct timeval timeout = {6, 0};//3s
        int ret = setsockopt(conn, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        if (ret < 0) {
            perror("setsockopt SO_SNDTIMEO");
            exit(1);
        }
        ret = setsockopt(conn, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        if (ret < 0) {
            perror("setsockopt SO_RCVTIMEO");
            exit(1);
        }

        socklen_t optlen = sizeof(struct timeval);

        struct timeval tv;

        memset(&tv, 0, sizeof(tv));

        getsockopt(conn, SOL_SOCKET, SO_RCVTIMEO, &tv, &optlen);
#endif
        static unsigned  int id=2;
        if (Server.add_forward(id,conn))
        {
            id++;
        } else
        {
            printf("forward conn failed!\n");
            close(conn);
        }

    }
    close(ss);
    ThreadPool::pool_destroy();
    forward::forward_pool_destroy();
#ifdef _WIN64
    WSACleanup();
#endif
    return 0;
}
