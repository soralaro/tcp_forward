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
    typedef enum {
        com_wait_star=0,
        com_head_rcv,
        com_head_rcv_end,
        com_data_rcv
    }COMMANT_STATE;
public:
    command_process(BlockQueue<MSG> *q_msg);
    ~command_process();
    void process(unsigned char *data_in, unsigned int len);
    void relase();
    void erease_mforwar(unsigned int socket_id );
    BlockQueue<MSG> *q_client_msg;
private:
    void rcv_comm_process(COMMANT com,MSG Msg);
    COMMANT command;
    int commant_cur;
    unsigned  int current_max_socket_id;
    COMMANT_STATE  state;
    std::map <unsigned int,forward *> mforward;
};

void command_process::erease_mforwar(unsigned int socket_id) {
    auto iter=mforward.find(socket_id);
    if(iter!=mforward.end())
    {
        auto forword = iter->second;
        forword->setEnd();
        mforward.erase(iter);
    }
}

command_process::command_process(BlockQueue<MSG> *q_msg)
{
    state =com_wait_star;
    commant_cur=0;
    current_max_socket_id=0;
    q_client_msg=q_msg;
}

command_process::~command_process()
{
    auto iter=mforward.find(command.socket_id);
    for(auto iter=mforward.begin(); iter!=mforward.end(); )
    {
        auto forword = iter->second;
        forword->setEnd();
        iter=mforward.erase(iter);
    }
}

command_process::relase()
{
    auto iter=mforward.find(command.socket_id);
    for(auto iter=mforward.begin(); iter!=mforward.end(); )
    {
        auto forword = iter->second;
        forword->setEnd();
        iter=mforward.erase(iter);
    }
    state =com_wait_star;
    commant_cur=0;
    current_max_socket_id=0;
}

void command_process::process(unsigned char *data_in, unsigned int len) {

    unsigned char *buf=data_in;
    unsigned int   pro_len=len;
     while (pro_len>0) {
         MSG Msg;
         Msg.size=0;
        switch (state) {
            case com_wait_star: {
                if (pro_len >= sizeof(command)) {
                    memcpy(&command, buf, sizeof(command));
                    buf += sizeof(command);
                    pro_len -= sizeof(command);
                    state = com_head_rcv_end;
                    commant_cur = sizeof(command);
                } else {
                    memcpy((unsigned char *) (&command), buf, pro_len);
                    buf += pro_len;
                    commant_cur = pro_len;
                    pro_len = 0;
                    state = com_head_rcv;
                }
                break;
            }
            case com_head_rcv: {
                unsigned int head_remain = sizeof(command) - commant_cur;
                if (pro_len >= head_remain) {
                    memcpy((unsigned char *) (&command) + commant_cur, buf, head_remain);
                    buf += head_remain;
                    pro_len -= head_remain;
                    state = com_head_rcv_end;
                    commant_cur = sizeof(command);
                } else {
                    memcpy((unsigned char *) (&command) + commant_cur, buf, pro_len);
                    buf += pro_len;
                    commant_cur += pro_len;
                    pro_len = 0;
                    state = com_head_rcv;
                }
                break;
            }
            case com_head_rcv_end: {
                Msg.type = MSG_TPY::msg_server_rcv;
                Msg.msg = buf;
                unsigned int commant_remain = command.size - sizeof(command);
                if (pro_len >= commant_remain) {
                    Msg.size = commant_remain;
                    buf += commant_remain;
                    pro_len -= commant_remain;
                    commant_cur = 0;
                    state = com_wait_star;
                } else {
                    Msg.size = pro_len;
                    buf += pro_len;
                    pro_len = 0;
                    commant_cur += pro_len;
                    state = com_data_rcv;
                }
                break;
            }
            case com_data_rcv: {
                Msg.type = MSG_TPY::msg_server_rcv;
                Msg.msg = buf;
                unsigned int commant_remain = command.size - sizeof(command) - commant_cur;
                if (pro_len >= commant_remain) {
                    Msg.size = commant_remain;
                    buf += commant_remain;
                    pro_len -= commant_remain;
                    commant_cur = 0;
                    state = com_wait_star;
                } else {
                    Msg.size = pro_len;
                    buf += pro_len;
                    pro_len = 0;
                    commant_cur += pro_len;
                    state = com_data_rcv;
                }
                break;
            }
            cdefault:
                break;
        }
         rcv_comm_process(command,Msg);

    }
}

void command_process::rcv_comm_process(COMMANT com,MSG Msg)
{
    auto iter=mforward.find(command.socket_id);
    switch(com.com)
    {

        case (unsigned int )socket_command::Data:
        {
            if(Msg.size>0)
            {
                if(iter==mforward.end())
                {
                    if(com.socket_id<=current_max_socket_id)
                    {
                        break;
                    }
                    forward *Forward=forward::forward_pool_get();
                    if(Forward!=NULL)
                    {
                        current_max_socket_id=com.socket_id;
                        Forward->init(com.socket_id,q_client_msg);
                        Forward->send_all(Msg);
                        mforward.insert(std::pair <unsigned int,forward *>(com.socket_id,Forward));
                    }
                } else {
                    auto forword = iter->second;
                    forword->send_all(Msg);
                }
            }
            break;
        }
        case (unsigned int )socket_command::connect:
        {
            if(iter==mforward.end())
            {
                if(com.socket_id<=current_max_socket_id)
                {
                        break;
                }
                forward *Forward=forward::forward_pool_get();
                if(Forward!=NULL)
                {
                    Forward->init(com.socket_id,q_client_msg);
                    current_max_socket_id=com.socket_id;
                    mforward.insert(std::pair <unsigned int,forward *>(com.socket_id,Forward));
                }
            }
            break;
        }
        case (unsigned int )socket_command::dst_connetc:
        {
            if(iter!=mforward.end())
            {
                auto forword = iter->second;
                forword->setEnd();
                mforward.erase(iter);
            }
            break;
        }
    }
}

#endif //GFW_COMMAND_PROCESS_H
