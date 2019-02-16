//
// Created by deepglint on 19-1-13.
//
#include "command_process.h"
#include "encrypt.h"


command_process::command_process(BlockQueue<MSG_COM> *q_msg)
{
    state =com_wait_star;
    commant_cur=0;
    current_max_socket_id=0;
    q_client_msg=q_msg;
    encry_rcv_len=0;
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

void command_process::data_encrypt(unsigned char *buf, unsigned int cur,int len)
{

    if(encry_data==NULL)
    {
        return;
    }
    if(!(*get_encrypt_state))
    {
        for(int i=0;i<len;i++)
        {
            buf[i]=(buf[i]^encryp_key);
        }
        return;
    }
    unsigned char *enp=encry_data+cur;
    for(int i=0;i<len;i++)
    {
       buf[i]=(buf[i]^enp[i]);
    }
}

void command_process::encrypt_code_resolv()
{
    unsigned char a=encry_data[encryp_key_2];
    a=a^encryp_key;
    for(int i=sizeof(COMMANT);i<encry_rcv_len;i++)
    {
        encry_data[i]=(encry_data[i]^encryp_key_2^a);
    }
    encry_data[encryp_key_2]=a^encryp_key;
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
    encry_rcv_len=0;
}

void command_process::process(unsigned char *data_in, unsigned int len) {

    unsigned char *buf=data_in;
    unsigned int   pro_len=len;
    while (pro_len>0) {
        MSG_COM Msg;
        Msg.size=0;
        switch (state) {
            case com_wait_star: {
                if (pro_len >= sizeof(command)) {
                    memcpy(&command, buf, sizeof(command));
                    des_decrypt((u_char *)&command,sizeof(command));
                    data_encrypt((unsigned char *)(&command),0,sizeof(command));
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
                    des_decrypt((u_char *)&command,sizeof(command));
                    data_encrypt((unsigned char *)(&command),0,sizeof(command));
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
                    data_encrypt(buf,commant_cur,commant_remain);
                    buf += commant_remain;
                    pro_len -= commant_remain;
                    commant_cur = 0;
                    DGDBG(" command_process state = com_wait_star,pro_len=%d",pro_len);
                    state = com_wait_star;
                } else {
                    Msg.size = pro_len;
                    data_encrypt(buf,commant_cur,pro_len);
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
                    data_encrypt(buf,commant_cur,commant_remain);
                    buf += commant_remain;
                    pro_len -= commant_remain;
                    commant_cur = 0;
                    DGDBG(" command_process state = com_wait_star pro_len=%d",pro_len);
                    state = com_wait_star;
                } else {
                    Msg.size = pro_len;
                    data_encrypt(buf,commant_cur,pro_len);
                    buf += pro_len;
                    commant_cur += pro_len;
                    pro_len = 0;
                    DGDBG(" command_process state = com_data_rcv ");
                    state = com_data_rcv;
                }
                break;
            }
        }
        if(state!=com_head_rcv)
            rcv_comm_process(Msg);
    }
}

void command_process::rcv_comm_process(MSG_COM Msg)
{
    auto iter=mforward.find(command.socket_id);
    DGDBG("rcv_comm_process_HEAD size=%x,sn=%x,id=%x,com=%x ",command.size,command.sn,command.socket_id,command.com);
    switch(command.com)
    {
        case (unsigned int )socket_command::Data:
        {
            DGDBG("rcv_comm_process,command=Data,socket_id=%d,size=%d",command.socket_id,Msg.size);
            if(Msg.size>0)
            {
                if(iter==mforward.end())
                {
                    MSG_COM Msg_sv;
                    Msg_sv.type = MSG_TPY::msg_socket_end;
                    char *buffer = new char[BUFFER_SIZE];
                    Msg_sv.socket_id=command.socket_id;
                    Msg_sv.msg = buffer;
                    Msg_sv.size=sizeof(COMMANT);

                    COMMANT commant;
                    commant.size=sizeof(COMMANT);
                    commant.com=(unsigned int)socket_command::dst_connetc;
                    commant.socket_id=command.socket_id;
                    memcpy(buffer,&commant,sizeof(commant));
                    q_client_msg->push(Msg_sv);

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
        case (unsigned int )socket_command::encrypt:
        {
            DGDBG("rcv_comm_process,command=encrypt,socket_id=%d",command.socket_id);
            memcpy(encry_data+encry_rcv_len,Msg.msg,Msg.size);
            encry_rcv_len+=Msg.size;
            if(state == com_wait_star)
            {
                encrypt_code_resolv();
                *get_encrypt_state=true;
                DGDBG("rcv_comm_process,command=encrypt,socket_id=%d,encry_rcv_len=%d",command.socket_id,encry_rcv_len);
                for(unsigned int i=0;i<encry_rcv_len;i++)
                    printf("%x ",encry_data[i]);
                printf("\n");
            }
            break;
        }
        default:
        {
            DGDBG("rcv_comm_process,command=default=%x,command.size=%x,socket_id=%x",command.com,command.size,command.socket_id);
            break;
        }
    }
}
