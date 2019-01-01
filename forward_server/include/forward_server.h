//
// Created by czx on 18-12-19.
//

#ifndef PROJECT_FORWAR_SERVER_H
#define PROJECT_FORWAR_SERVER_H
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
#include <queue>
#include <semaphore.h>
#include <signal.h>
#include <netinet/tcp.h>
#include <map>
#include "gdb_log.h"
#include "thread_pool.h"
#include "block_queue.h"
#define BUFFER_SIZE 1024

 enum class MSG_TPY
{
    msg_client_rcv=0,
    msg_server_rcv,
    msg_socket_end
};

typedef struct MSG_struct
{
    MSG_TPY type;
    void * msg;
    int  size;
}MSG;

class forward_server
{
public:
    forward_server(int g_id);
    void init(int socket_int);
    ~forward_server();
    void release();
    std::string server_ip;
    bool exit_;
    static  ThreadPool threadPool;
    static void forward_pool_int(int max_num);
    static forward_server * forward_pool_get();
    static void forward_pool_destroy();
    int free;
private:
    static std::vector<forward_server *> forward_Pool;
    static void data_cover(unsigned char *buf, int len);
    static void client_rcv(void *arg);
    static void client_forward(void *arg);
    static void server_rcv(void *arg);
    static void server_forward(void *arg);
    bool server_connect();
    static int send_all(int socket, char *buf,int size);
    int client_socket;
    int server_socket;
    struct sockaddr_in servaddr;
    int server_port;
private:
    BlockQueue<MSG> q_server_msg;
    BlockQueue<MSG> q_client_msg;
    bool client_rcv_end;
    bool client_forward_end;
    bool server_rcv_end;
    bool server_forward_end;
    sem_t sem_end_;
    bool end_;
public:
    int id;
private:
};



#endif //PROJECT_FORWAR_SERVER_H