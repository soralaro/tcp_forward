//
// Created by deepglint on 19-1-13.
//
#include "command_process.h"

void command_process::erease_mforward(unsigned int socket_id) {
    auto iter=mforward.find(socket_id);
    if(iter!=mforward.end())
    {
        DGDBG("erease_mforward socket_id=%d",socket_id);
        auto forword = iter->second;
        forword->setEnd();
        mforward.erase(iter);
    }
}

command_process::command_process(BlockQueue<MSG> *q_msg)
{
    state =com_wait_star;
    commant_cur=0;
    q_client_msg=q_msg;
    current_max_socket_id=0;
}

command_process::~command_process()
{
    for(auto iter=mforward.begin(); iter!=mforward.end(); )
    {
        auto forword = iter->second;
        forword->setEnd();
        iter=mforward.erase(iter);
    }
}

void command_process::relase()
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
                    DGDBG(" command_process state = com_head_rcv_end");
                    if(command.size>sizeof(command)) {
                        state = com_head_rcv_end;
                        commant_cur = sizeof(command);
                    } else
                    {
                        state = com_wait_star;
                        commant_cur=0;
                    }
                } else {
                    memcpy((unsigned char *) (&command), buf, pro_len);
                    buf += pro_len;
                    commant_cur = pro_len;
                    pro_len = 0;
                    DGDBG(" command_process state = com_head_rcv");
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
                    DGDBG(" command_process state = com_head_rcv_end");
                    if(command.size>sizeof(command)) {
                        state = com_head_rcv_end;
                        commant_cur = sizeof(command);
                    } else
                    {
                        state = com_wait_star;
                        commant_cur=0;
                    }
                } else {
                    memcpy((unsigned char *) (&command) + commant_cur, buf, pro_len);
                    buf += pro_len;
                    commant_cur += pro_len;
                    pro_len = 0;
                    DGDBG(" command_process state = com_head_rcv");
                    state = com_head_rcv;
                }
                break;
            }
            case com_head_rcv_end: {
                Msg.type = MSG_TPY::msg_server_rcv;
                Msg.msg = buf;
                unsigned int commant_remain = command.size - sizeof(command);
                DGDBG(" command_process state =com_head_rcv_end id=%d,command.size=%dpro_len=%d ",command.socket_id,command.size,pro_len);
                if (pro_len >= commant_remain) {
                    Msg.size = commant_remain;
                    buf += commant_remain;
                    pro_len -= commant_remain;
                    commant_cur = 0;
                    DGDBG(" command_process state = com_wait_star id=%d command.size=%d",command.socket_id,command.size);
                    state = com_wait_star;
                } else {
                    Msg.size = pro_len;
                    buf += pro_len;
                    commant_cur += pro_len;
                    pro_len = 0;
                    DGDBG(" command_process state = com_data_rcv");
                    state = com_data_rcv;
                }
                break;
            }
            case com_data_rcv: {
                Msg.type = MSG_TPY::msg_server_rcv;
                Msg.msg = buf;
                unsigned int commant_remain = command.size - commant_cur;
                if (pro_len >= commant_remain) {
                    Msg.size = commant_remain;
                    buf += commant_remain;
                    pro_len -= commant_remain;
                    commant_cur = 0;
                    DGDBG(" command_process state = com_wait_star");
                    state = com_wait_star;
                } else {
                    Msg.size = pro_len;
                    buf += pro_len;
                    pro_len = 0;
                    commant_cur += pro_len;
                    DGDBG(" command_process state = com_data_rcv");
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
    DGDBG("rcv_comm_process com=%d,socket_id=%d",com.com,command.socket_id);
    switch(com.com)
    {

        case (unsigned int )socket_command::Data:
        {
            DGDBG("rcv_comm_process Data size=%d",Msg.size);
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
                DGDBG("rcv_comm_process socket_command::connect socket_id:%d",com.socket_id);
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
                DGDBG("rcv_comm_process socket_command::dst_connetc socket_id:%d",com.socket_id);
                auto forword = iter->second;
                forword->setEnd();
                mforward.erase(iter);
            } else
            {
                DGDBG("rcv_comm_process socket_command::dst_connetc socket_id:%d,not exit",com.socket_id);
            }
            break;
        }
        default:
            {
                DGDBG("socket_command: %d",com.com);
                break;
            }
    }
}
