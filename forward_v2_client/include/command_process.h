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

class command_process
{
    typedef enum {
        com_wait_star=0,
        com_head_rcv,
        com_head_rcv_end,
        com_data_rcv,
    }COMMANT_STATE;
public:
    command_process(BlockQueue<MSG> *q_msg);
    ~command_process();
   void  process(unsigned char *data_in, unsigned int len);
   void  erease_mforward(void *from);
   void  add_mforward(forward * Forward);
   void relase();

private:
    COMMANT command;
    int commant_cur;
    COMMANT_STATE  state;
    std::map <unsigned int,forward *> mforward;
    unsigned  int current_max_socket_id;
    BlockQueue<MSG> *q_client_msg;
    void rcv_comm_process(MSG Msg);
};

#endif //GFW_COMMAND_PROCESS_H
