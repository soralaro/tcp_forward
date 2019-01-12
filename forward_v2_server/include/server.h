//
// Created by czx on 18-12-19.
//
#ifndef PROJECT_SERVER_H
#define PROJECT_SERVER_H
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
#include "ringbuf.h"
#include "../include/command_process.h"

class server
{
public:
    server();
    void init(unsigned int g_id,int socket_int,std::string ip ,int port);
    ~server();
    void release();
    static void setKey(unsigned  char input_key);
    static void server_pool_int(int max_num);
    static void server_pool_destroy();
    static server * server_pool_get();
    int id;
    bool free;
    bool destroy;
    BlockQueue<MSG> q_client_msg;
private:
    static void data_cover(unsigned char *buf, int len);
    static void server_rcv(void *arg);
    bool server_connect();
    static int send_all(int socket, char *buf,int size);
    int client_socket;
    struct sockaddr_in servaddr;
private:
    static std::vector<server *> server_Pool;
    bool end_;
    static  unsigned  char encryp_key;
    command_process *commandProcess;
private:
    std::mutex mutex_client_socket;
    std::condition_variable cond_client_socket;
};



#endif //PROJECT_SERVER_H