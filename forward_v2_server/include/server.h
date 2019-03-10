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
#include "../include/command_process.h"

class server
{
public:
    server();
    void init(unsigned int g_id,int socket_int);
    ~server();
    void release();
    static void setKey(unsigned  char input_key);
    static void setKey_2(unsigned  char input_key){encryp_key_2=input_key;};
    static void setDesKey(char *key);
    static void setDesKey_2(char *key);
    static void server_pool_int(int max_num);
    static void server_pool_destroy();
    static server * server_pool_get();
    int id;
    bool free;
    bool destroy;
    BlockQueue<MSG> q_client_msg;
private:
    void data_cover(unsigned char *buf, int len);
    void data_encrypt(unsigned char *buf, int len);
    static void server_rcv(void *arg);
    static void  forward(void *arg);
    bool server_connect();
    int send_all(char *buf,int size);
    static void timer_fuc(void *arg);
    int client_socket;
private:
    static std::vector<server *> server_Pool;
    bool rcv_end;
    bool forward_end;
    bool end_;
    static  unsigned  char encryp_key;
    static  unsigned  char encryp_key_2;
    static char des_key[17];
    static char des_key_2[17];
    unsigned char *encry_data;
    command_process *commandProcess;
    unsigned int send_sn;
    char heart_beat;
    unsigned int connect_exist_time;
    int usr_id;
private:
    std::mutex mutex_client_socket;
    std::condition_variable cond_client_socket;

    std::mutex mutex_forward_start;
    std::condition_variable cond_forward_start;

    std::mutex mutex_forward_end;
    std::condition_variable cond_forward_end;
};



#endif //PROJECT_SERVER_H