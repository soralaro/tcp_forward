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
    void init(std::string ip ,int port);
    ~server();
    void release();
    static void setKey(unsigned  char input_key);
    static void setKey_2(unsigned  char input_key){encryp_key_2=input_key;};
    static void setDesKey(char *key){memcpy(des_key,key,sizeof(des_key)-1);};
    bool add_forward(unsigned int g_id,int socket_int);
    int id;
    BlockQueue<MSG> q_client_msg;
private:
    static void data_cover(unsigned char *buf, int len);
    void data_encrypt(unsigned char *buf, int len);
    static void server_rcv(void *arg);
    static void server_forward(void *arg);
    bool server_connect();
    static int send_all(int socket, char *buf,int size);
    static void timer_fuc(void *arg);
    int server_socket;
    struct sockaddr_in servaddr;
private:
    bool connect_state;
    bool get_encrypt_state;
    bool end_;
    bool forward_end;
    static  unsigned  char encryp_key;
    static  unsigned  char encryp_key_2;
    static char des_key[17];
    unsigned char encry_data[BUFFER_SIZE];
    command_process *commandProcess;
    unsigned int send_sn;
    char heart_beat;
private:
    std::mutex mutex_connect;
    std::condition_variable cond_connect;
};



#endif //PROJECT_SERVER_H