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

void command_process::erease_mforward(unsigned int socketid) {
    auto iter=mforward.find(socketid);
    if(iter!=mforward.end())
    {
        auto forword = iter->second;
        forword->setEnd();
        DGDBG("erease_mforward socketid=%d",socketid);
        mforward.erase(iter);
    } else
    {
        DGDBG("erease_mforward socketid=%d,not exit",socketid);
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
                    DGDBG(" command_process state = com_head_rcv_end,pro_len=%d",pro_len);
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
                    DGDBG(" command_process state = com_head_rcv");
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
                    DGDBG(" command_process state = com_head_rcv_end,pro_len=%d",pro_len);
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
                    DGDBG(" command_process state = com_head_rcv");
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
                    DGDBG(" command_process state = com_wait_star,pro_len=%d",pro_len);
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
                    DGDBG(" command_process state = com_wait_star pro_len=%d",pro_len);
                    state = com_wait_star;
                } else {
                    Msg.size = pro_len;
                    buf += pro_len;
                    pro_len = 0;
                    commant_cur += pro_len;
                    DGDBG(" command_process state = com_data_rcv ");
                    state = com_data_rcv;
                }
                break;
            }
        }
        rcv_comm_process(Msg);
    }
}

void command_process::rcv_comm_process(MSG Msg)
{
    auto iter=mforward.find(command.socket_id);
    switch(command.com)
    {
        case (unsigned int )socket_command::Data:
        {
            DGDBG("rcv_comm_process,command=Data,socket_id=%d,size=%d",command.socket_id,Msg.size);
            if(Msg.size>0)
            {
                if(iter==mforward.end())
                {
                    Msg.type = MSG_TPY::msg_socket_end;
                    char *buffer = new char[BUFFER_SIZE];
                    Msg.socket_id=command.socket_id;
                    Msg.msg = buffer;
                    COMMANT *commant=(COMMANT *) buffer;
                    commant->size=sizeof(COMMANT);
                    commant->com=(unsigned int)socket_command::dst_connetc;
                    commant->socket_id=command.socket_id;
                    q_client_msg->push(Msg);

                } else {
                    auto forword = iter->second;
                    DGDBG("forword->send_all size=%d \n",Msg.size);
                    forword->send_all((char *)Msg.msg,Msg.size);
                }
            }
            break;
        }
        case (unsigned int )socket_command::connect:
        {
            DGDBG("rcv_comm_process,command=connect,socket_id=%d",command.socket_id);
            break;
        }
        case (unsigned int )socket_command::dst_connetc:
        {
            DGDBG("rcv_comm_process,command=dst_connetc,socket_id=%d",command.socket_id);
            if(iter!=mforward.end())
            {
                DGDBG("rcv_comm_process,mforward.erase,iter->first=%d",iter->first);
                auto forword = iter->second;
                forword->setEnd();
                mforward.erase(iter);
            }
            break;
        }
        default:
        {
            DGDBG("rcv_comm_process,command=default=%d,command.size=%d,socket_id=%d",command.com,command.size,command.socket_id);
            break;
        }
    }
}
