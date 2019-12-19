//
// Created by czx on 19-1-7.
//

#ifndef GFW_COMMAND_PROCESS_H
#define GFW_COMMAND_PROCESS_H
#include <sys/types.h>
#ifdef _WIN64  
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/shm.h>
#include <netinet/tcp.h>
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
#include "forward.h"

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
    command_process(BlockQueue<MSG_COM> *q_msg);
    ~command_process();
   void  process(unsigned char *data_in, unsigned int len);
   void  erease_mforward(unsigned int sockeid);
   bool check_mforward_exist(unsigned int socketid);
   void  add_mforward(forward * Forward);
   int  get_mforward_size();
   void relase();
   bool *get_encrypt_state;
   unsigned  char encryp_key_2;
   unsigned  char encryp_key;
   unsigned char *encry_data;
   unsigned int encry_rcv_len;
private:
    COMMANT command;
    int commant_cur;
    COMMANT_STATE  state;
    u_char  command_Buf[BUFFER_SIZE+sizeof(COMMANT)+1];
    std::map <unsigned int,forward *> mforward;
    unsigned  int current_max_socket_id;
    BlockQueue<MSG_COM> *q_client_msg;
    void rcv_comm_process(MSG_COM Msg);
    void data_encrypt(unsigned char *buf, unsigned int cur,int len);
    void encrypt_code_resolv();
};

#endif //GFW_COMMAND_PROCESS_H
