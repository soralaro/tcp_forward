//
// Created by czx on 18-12-19.
//

#ifndef PROJECT_FORWAR_H
#define PROJECT_FORWAR_H
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


class forward
{
public:
    forward(struct sockaddr_in addr);
    void init(unsigned int g_id,BlockQueue<MSG> *q_msg);
    ~forward();
    void release();
    int send_all(MSG msg);
    static void forward_pool_int(int max_num,struct sockaddr_in addr);
    static forward * forward_pool_get();
    static void forward_pool_destroy();
    static void setKey(unsigned  char input_key);
    void setEnd(){end_=true;};
    bool free;
    bool destroy;
    unsigned int id;
private:
    static std::vector<forward *> forward_Pool;
    static void server_rcv(void *arg);
    bool server_connect();

    BlockQueue<MSG> *q_client_msg;
    BlockQueue<MSG> q_send_msg;
    int server_socket;
    struct sockaddr_in servaddr;

    bool connect_state;
    std::mutex mutex_connect;
    std::condition_variable cond_connect;
    std::mutex mutex_server_socket;
    std::condition_variable cond_server_socket;;
private:
    bool server_rcv_end;
    bool end_;
    static  unsigned  char encryp_key;
private:
};



#endif //PROJECT_FORWAR_H