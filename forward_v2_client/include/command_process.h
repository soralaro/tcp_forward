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
#include "ringbuf.h"
#include "forward.h"

class command_process
{
    enum {
        com_wait_star=0,
        com_head_rcv,
        com_head_rcv_end,
        com_data_rcv,
    }COMMANT_STATE;
public:
    void command_process();
    void ~command_process();
    process(unsigned char *data_in, unsigned int len);
    BlockQueue<MSG> *q_server_msg;
private:
    COMMANT commant;
    int commant_cur;
    COMMANT_STATE  state;
    std::map <unsigned int,forward *> mforward;
};

void command_process::command_process()
{
     state =com_wait_star;
    commant_cur=0;
}

void command_process::~command_process()
{

}

int command_process::process(unsigned char *data_in, unsigned int len) {

    unsigned char *buf=data_in;
    unsigned int   pro_len=len;
     while (pro_len>0) {
         MSG Msg;
         Msg.size=0;
        switch (state) {
            case com_wait_star:
                if(pro_len>=sizeof(commant)) {
                    memcpy(&command, buf, sizeof(commant));
                    buf+=sizeof(commant);
                    pro_len-=sizeof(commant);
                    state=com_head_rcv_end;
                    commant_cur=sizeof(commant);
                }
                else
                {
                    memcpy((unsigned char *)(&command), buf, pro_len);
                    buf+=pro_len;
                    commant_cur=pro_len;
                    pro_len=0;
                    state=com_head_rcv;
                }
                break;
            case com_head_rcv:
                unsigned int head_remain=sizeof(commant)-commant_cur;
                if(pro_len>=head_remain) {
                    memcpy((unsigned char *)(&command)+commant_cur, buf, head_remain);
                    buf+=head_remain;
                    pro_len-=head_remain
                    state=com_head_rcv_end;
                    commant_cur=sizeof(commant);
                }
                else
                {
                    memcpy((unsigned char *)(&command)+commant_cur, buf, pro_len);
                    buf+=pro_len;
                    commant_cur+=pro_len;
                    pro_len=0;
                    state=com_head_rcv;
                }
                break;

            case com_head_rcv_end:
                Msg.type = MSG_TPY::msg_server_rcv;
                Msg.msg = buf;
                unsigned int commant_remain=commant.size-sizeof(commant);
                if(pro_len>=commant_remain) {
                    Msg.size = commant_remain;
                    buf+=commant_remain;
                    pro_len-=commant_remain;
                    commant_cur=0;
                    state=com_wait_star;
                } else
                {
                   Msg.size=pro_len;
                    buf+=pro_len;
                   pro_len=0;
                   commant_cur+=pro_len;
                   state=com_data_rcv;
                }
                break;
            case com_data_rcv:
                Msg.type = MSG_TPY::msg_server_rcv;
                Msg.msg = buf;
                unsigned int commant_remain=commant.size-sizeof(commant)-commant_cur;
                if(pro_len>=commant_remain) {
                    Msg.size = commant_remain;
                    buf+=commant_remain;
                    pro_len-=commant_remain;
                    commant_cur=0;
                    state=com_wait_star;
                } else
                {
                    Msg.size=pro_len;
                    buf+=pro_len;
                    pro_len=0;
                    commant_cur+=pro_len;
                    state=com_data_rcv;
                }
                break;
            case defaut:
                break;
        }
        if(Msg.size>0)
        {
            mforward.at(commant.socket_id)->send_all((char *)Msg.msg,Msg.size);
        }
    }
}

#endif //GFW_COMMAND_PROCESS_H
