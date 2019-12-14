//
// Created by czx on 18-12-19.
//

#ifndef PROJECT_SERVER_H
#define PROJECT_SERVER_H
#include <sys/types.h>
#ifdef _WIN64  
#include <winsock.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/shm.h>
#endif
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <thread>
#include <iostream>
#include <queue>
#include <semaphore.h>
#include <signal.h>
#include <map>
#include "gdb_log.h"
#include "thread_pool.h"
#include "block_queue.h"
#include "command.h"
#include "../include/command_process.h"
#define  DISCONNECT 0
#define  CONNECTED  1
#define  CONNECTING 2
#define  CONNECT_WAIT 3
#define  IDEL_TIME_MAX   10
class server
{
public:
    server();
    void init(std::string ip ,int port);
    ~server();
    void release();
    static unsigned int user_id;
    static void setKey(unsigned  char input_key);
    static void setKey_2(unsigned  char input_key){encryp_key_2=input_key;};
    static void setDesKey(char *key){memcpy(des_key,key,sizeof(des_key)-1);};
    static void setDesKey_2(char *key){memcpy(des_key_2,key,sizeof(des_key_2)-1);};
    bool add_forward(unsigned int g_id,int socket_int);
    int id;
    BlockQueue<MSG_COM> q_client_msg;
private:
    static void data_cover(unsigned char *buf, int len);
    void data_encrypt(unsigned char *buf, int len);
    static void server_rcv(void *arg);
    static void server_forward(void *arg);
    bool server_connect();
    static int send_all(int socket, char *buf,int size);
    static void timer_fuc(void *arg);
    void heart_beat_set(int value);
    int heart_beat_get();
    void idel_time_set(int value);
    int idel_time_get();

    int server_socket;
    struct sockaddr_in servaddr;
private:
    std::mutex connect_lock;
    int connect_state;
    bool get_encrypt_state;
    bool end_;
    bool forward_end;
    static  unsigned  char encryp_key;
    static  unsigned  char encryp_key_2;
    static char des_key[17];
    static char des_key_2[17];
    unsigned char encry_data[BUFFER_SIZE];
    command_process *commandProcess;
    unsigned int send_sn;
    std::mutex heart_beat_mutex;
    int heart_beat;
    std::mutex idel_time_mutex;
    int idel_time;
private:
    std::mutex mutex_connect;
    std::condition_variable cond_connect;
    std::mutex mutex_notic_connect;
    std::condition_variable cond_notic_connect;
};



#endif //PROJECT_SERVER_H