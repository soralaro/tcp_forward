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

class server
{
public:
    server();
    void init(std::string ip ,int port);
    ~server(){};
    void release();
    static void setKey(unsigned  char input_key);
    int id;
    BlockQueue<MSG> q_server_msg;
    BlockQueue<MSG> q_client_msg;
    RingBuffer ringBuffer;
private:
    static void data_cover(unsigned char *buf, int len);
    static void server_rcv(void *arg);
    static void server_forward(void *arg);
    bool server_connect();
    static int send_all(int socket, char *buf,int size);
    int server_socket;
    struct sockaddr_in servaddr;
private:

    bool connect_state;
    bool end_;
    static  unsigned  char encryp_key;
private:
    std::mutex mutex_connect;
    std::condition_variable cond_connect;
};
server::server()
{
    end_=false;
    id=0;
    connect_state=false;
    server_socket=0;
    memset(&servaddr,0,sizeof(servaddr));
}


#endif //PROJECT_SERVER_H