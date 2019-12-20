//
// Created by czx on 19-1-7.
//

#ifndef GFW_COMMAND_PROCESS_H
#define GFW_COMMAND_PROCESS_H
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
#include "forward.h"
#ifdef USEMSQL
#include "mysql.h"
#else 
#include "mySqlite3.h"
#endif
class command_process
{
    typedef enum {
        com_wait_star=0,
        com_head_rcv,
        com_head_rcv_end,
        com_data_rcv,
        com_data_rcv_end,
        com_ext_rcv
    }COMMANT_STATE;
public:
    command_process(BlockQueue<MSG> *q_msg);
    ~command_process();
    void process(unsigned char *data_in, unsigned int len);
    void relase();
    void erease_mforward(unsigned int socket_id);
    bool check_mforward_exist(unsigned int socketid);
    void log(int login_out,int usrId,int dev_num);
    BlockQueue<MSG> *q_client_msg;
    unsigned char *encry_data;
    bool *server_end;
    int *usr_id;
    struct sockaddr_in *client_addr;
    std::map<int ,int> *mapUsr;
    std::mutex *mapUsr_lock;
private:
    void rcv_comm_process(COMMANT com,MSG Msg);
    void data_encrypt(unsigned char *buf,unsigned int cur, int len);
    COMMANT command;
    int commant_cur;
    unsigned int current_max_socket_id;
    COMMANT_STATE  state;
    u_char  command_Buf[BUFFER_SIZE+sizeof(COMMANT)+1];
    std::map <unsigned int,forward *> mforward;
    unsigned int max_sn;
   // mySQL mysql;
    mySqlite3 mysql;
};

#endif //GFW_COMMAND_PROCESS_H
