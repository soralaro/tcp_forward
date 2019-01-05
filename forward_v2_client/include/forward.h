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
#include "command.h"
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

class forward
{
public:
    forward(int g_id);
    void init(int socket_int,std::string ip ,int port);
    ~forward();
    void release();
    static  ThreadPool threadPool;
    static void forward_pool_int(int max_num);
    static forward * forward_pool_get();
    static void forward_pool_destroy();
    static void setKey(unsigned  char input_key);
    int free;
    int id;
private:
    static std::vector<forward *> forward_Pool;
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
private:
    BlockQueue<MSG> q_server_msg;
    BlockQueue<MSG> q_client_msg;
    bool client_rcv_end;
    bool client_forward_end;
    bool server_rcv_end;
    bool server_forward_end;
    sem_t sem_end_;
    bool end_;
    static  unsigned  char encryp_key;
private:
};



#endif //PROJECT_FORWAR_SERVER_H