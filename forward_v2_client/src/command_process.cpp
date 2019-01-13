//
// Created by deepglint on 19-1-13.
//
#include "command_process.h"


command_process::command_process(BlockQueue<MSG> *q_msg)
{
    state =com_wait_star;
    commant_cur=0;
    current_max_socket_id=0;
    q_client_msg=q_msg;
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

void command_process::erease_mforward(void *from) {
    unsigned int socket_id=((forward *)from)->id;
    auto iter=mforward.find(socket_id);
    if(iter!=mforward.end())
    {
        auto forword = iter->second;
        forword->setEnd();
        mforward.erase(iter);
    }
}

void command_process::add_mforward(forward * Forward)
{
    mforward.insert(std::pair<unsigned int,forward *>(Forward->id,Forward));
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
        }
        if(Msg.size>0)
        {
            rcv_comm_process(Msg);
        }
    }
}

void command_process::rcv_comm_process(MSG Msg)
{
    auto iter=mforward.find(command.socket_id);
    switch(command.com)
    {

        case (unsigned int )socket_command::Data:
        {
            if(Msg.size>0)
            {
                if(iter==mforward.end())
                {
                    Msg.type = MSG_TPY::msg_socket_end;
                    char *buffer = new char[BUFFER_SIZE];
                    Msg.from=NULL;
                    Msg.msg = buffer;
                    COMMANT *commant=(COMMANT *) buffer;
                    commant->size=sizeof(COMMANT);
                    commant->com=(unsigned int)socket_command::dst_connetc;
                    commant->socket_id=command.socket_id;
                    q_client_msg->push(Msg);

                } else {
                    auto forword = iter->second;
                    forword->send_all((char *)Msg.msg,Msg.size);
                }
            }
            break;
        }
        case (unsigned int )socket_command::connect:
        {
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
