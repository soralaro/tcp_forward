//
// Created by deepglint on 19-1-13.
//
#include "command_process.h"
#include "encrypt.h"

void command_process::data_encrypt(unsigned char *buf, unsigned int cur,int len)
{

    if(encry_data==NULL)
    {
        return;
    }
    unsigned char *enp=encry_data+cur;
    for(int i=0;i<len;i++)
    {
        buf[i]=(buf[i]^enp[i]);
    }

}

bool command_process::check_mforward_exist(unsigned int socketid)
{
    auto iter=mforward.find(socketid);
    if(iter!=mforward.end())
    {
        return true;
    }
    else
    {
        return false;
    }
}

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
    encry_data=NULL;
    server_end=NULL;
    max_sn=0;
    memset(command_Buf,0,sizeof(command_Buf));
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
    encry_data=NULL;
    server_end=NULL;
    max_sn=0;
}

void command_process::log(int login_out)
{
    time_t now = time(0);// 基于当前系统的当前日期/时间
    tm *tm_now = localtime(&now);
 
    char date_c[32];
    memset(date_c,0,sizeof(date_c));
    strftime(date_c, sizeof(date_c), "%Y-%m-%d", tm_now);
    std::string file_name;
    file_name="./log/";
    file_name=file_name+date_c+".txt";
    FILE *flog = fopen( file_name.c_str(), "a+" );
    if(flog!=NULL)
    {
        char strTime[32];
        strftime(strTime, sizeof(strTime), "%Y-%m-%d %H:%M:%S", tm_now);
        if(login_out==1)
            fprintf(flog, "%s %s id=%d login\n", strTime,inet_ntoa(client_addr->sin_addr),*usr_id);
        else
            fprintf(flog, "%s %s id=%d logout\n", strTime,inet_ntoa(client_addr->sin_addr),*usr_id);
        fclose(flog);
    }
}


void command_process::process(unsigned char *data_in, unsigned int len) {

    unsigned char *buf=data_in;
    unsigned int   pro_len=len;
    MSG Msg;
    while (pro_len>0) {
        if(server_end==NULL)
        {
            return;
        }
        Msg.size=0;
        Msg.type=MSG_TPY ::msg_res;
        switch (state) {
            case com_wait_star: {
                if (pro_len >= sizeof(command)) {
                    memcpy(&command, buf, sizeof(command));

                    des_decrypt_3((u_char *)&command,sizeof(command));

                    des_decrypt((u_char *)&command,sizeof(command));
                    data_encrypt((unsigned char *)(&command),0,sizeof(command));
                    buf += sizeof(command);
                    pro_len -= sizeof(command);

                    if((ALIGN_16(command.size)+command.ex_size)>sizeof(command)) {
                        state = com_head_rcv_end;
                        commant_cur = sizeof(command);
                        DGDBG(" command_process state = com_head_rcv_end pro_len=%d",pro_len);
                    } else
                    {
                        state = com_wait_star;
                        DGDBG(" command_process state = com_wait_star pro_len=%d",pro_len);
                        commant_cur=0;
                    }
                } else {
                    memcpy((unsigned char *) (&command), buf, pro_len);
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
                    des_decrypt_3((u_char *)&command,sizeof(command));
                    des_decrypt((u_char *)&command,sizeof(command));

                    data_encrypt((unsigned char *)(&command),0,sizeof(command));
                    buf += head_remain;
                    pro_len -= head_remain;
                    DGDBG(" command_process state = com_head_rcv_end");
                    if((ALIGN_16(command.size)+command.ex_size)>sizeof(command)) {
                        state = com_head_rcv_end;
                        commant_cur = sizeof(command);
                    } else
                    {
                        state = com_wait_star;
                        commant_cur=0;
                    }
                } else {
                    memcpy((unsigned char *) (&command) + commant_cur, buf, pro_len);
                    commant_cur += pro_len;
                    pro_len = 0;
                    DGDBG(" command_process state = com_head_rcv");
                    state = com_head_rcv;
                }
                break;
            }
            case com_head_rcv_end: {
                unsigned int commant_remain = ALIGN_16(command.size) - sizeof(command);
                if (pro_len >= commant_remain) {
                    Msg.size = commant_remain;
                    des_decrypt_3(buf,commant_remain);
                    des_decrypt_2(buf,commant_remain);
                    data_encrypt(buf,commant_cur,command.size - sizeof(command));
                    Msg.type = MSG_TPY::msg_server_rcv;
                    Msg.msg = buf;
                    Msg.size = command.size-sizeof(command);
                    buf += commant_remain;
                    pro_len -= commant_remain;
                    if(command.ex_size==0) {
                        commant_cur = 0;
                        state = com_wait_star;
                    } else{
                        commant_cur +=commant_remain;
                        state = com_data_rcv_end;
                    }
                    DGDBG(" command_process state = com_wait_star,pro_len=%d",pro_len);

                } else {
                    memcpy(command_Buf,buf,pro_len);
                    commant_cur += pro_len;
                    pro_len = 0;
                    DGDBG(" command_process state = com_data_rcv");
                    state = com_data_rcv;
                }
                break;
            }
            case com_data_rcv: {
                unsigned int commant_remain = ALIGN_16(command.size) - commant_cur;
                if (pro_len >= commant_remain) {
                    memcpy(command_Buf+commant_cur-sizeof(command),buf,commant_remain);
                    des_decrypt_3(command_Buf, ALIGN_16(command.size)-sizeof(command));
                    des_decrypt_2(command_Buf, ALIGN_16(command.size)-sizeof(command));
                    data_encrypt(command_Buf,sizeof(command),command.size-sizeof(command));
                    Msg.type = MSG_TPY::msg_server_rcv;
                    Msg.msg = command_Buf;
                    Msg.size = command.size-sizeof(command);
                    buf += commant_remain;
                    pro_len -= commant_remain;
                    if(command.ex_size==0)
                    {
                        commant_cur = 0;
                        state=com_wait_star;
                        DGDBG(" command_process state = com_wait_star pro_len=%d",pro_len);
                    } else {
                        commant_cur +=commant_remain;
                        state = com_data_rcv_end;
                        DGDBG(" command_process state = com_data_rcv_end pro_len=%d",pro_len);
                    }
                } else {
                    memcpy(command_Buf+commant_cur-sizeof(command),buf,pro_len);
                    commant_cur += pro_len;
                    pro_len = 0;
                    DGDBG(" command_process state = com_data_rcv ");
                    state = com_data_rcv;
                }
                break;
            }
            case com_data_rcv_end: {
                Msg.type=MSG_TPY::msg_ext_data;
                unsigned int ex_data_remain=command.ex_size;
                if (pro_len >= ex_data_remain) {
                    buf+=ex_data_remain;
                    pro_len-=ex_data_remain;
                    state = com_wait_star;
                    commant_cur=0;
                    DGDBG(" command_process state = com_wait_star pro_len=%d",pro_len);
                } else{
                    commant_cur+=pro_len;
                    pro_len=0;
                    state=com_ext_rcv;
                    DGDBG(" command_process state = com_ext_rcv ");
                }
                break;
            }
            case com_ext_rcv: {
                Msg.type=MSG_TPY::msg_ext_data;
                unsigned int ex_data_remain=command.ex_size+ALIGN_16(command.size)-commant_cur;
                if (pro_len >= ex_data_remain) {
                    buf+=ex_data_remain;
                    pro_len-=ex_data_remain;
                    state = com_wait_star;
                    commant_cur=0;
                    DGDBG(" command_process state = com_wait_star pro_len=%d",pro_len);
                } else{
                    commant_cur+=pro_len;
                    pro_len=0;
                    DGDBG(" command_process state = com_ext_rcv ");
                    state=com_ext_rcv;
                }
                break;
            }
            cdefault:
                break;
        }
        if((state!=com_head_rcv)&&(Msg.type!=MSG_TPY::msg_ext_data))
            rcv_comm_process(command,Msg);
    }
}

void command_process::rcv_comm_process(COMMANT com,MSG Msg)
{
    auto iter=mforward.find(command.socket_id);
    DGDBG("rcv_comm_process_HEAD size=%x,sn=%x,id=%x,com=%x ",command.size,command.sn,command.socket_id,command.com);
    if(command.sn>max_sn)
    {
        if(command.sn-max_sn>2)
        {
            *server_end=true;
            server_end=NULL;
            DGERR("rcv_comm_process_HEAD size=%x,sn=%x,id=%x,com=%x ",command.size,command.sn,command.socket_id,command.com);
            return;
        } else{
            max_sn=command.sn;
        }
    }
    if(com.size>1024) {
        *server_end=true;
        server_end=NULL;
        DGERR("rcv_comm_process_HEAD size=%x,sn=%x,id=%x,com=%x ",command.size,command.sn,command.socket_id,command.com);
        return;
    }
    if(*usr_id==-1)
    {
        *usr_id=com.user_id;
        if(mysql.query_expire(*usr_id))
        {
            MSG Msg_send;
            //add a user expire
            Msg_send.type = MSG_TPY::msg_client_expire;
            Msg_send.socket_id=1;
            char *buffer1 = new char[sizeof(COMMANT)];
            Msg_send.msg = buffer1;
            Msg_send.size=sizeof(COMMANT);
            COMMANT commant1;
            commant1.size=sizeof(COMMANT);
            commant1.com=(unsigned int)socket_command::user_expire;
            commant1.socket_id=1;
            memcpy(buffer1,&commant1,sizeof(COMMANT));
            q_client_msg->push(Msg_send);
        }
        else
        {
            auto iter = mapUsr->find(*usr_id);
            if(iter != mapUsr->end())
            {
                iter->second++;
                if(iter->second>ONE_USER_MAX_DEVICE)
                {
                    MSG Msg_send;
                    //add a user expire
                    Msg_send.type = MSG_TPY::msg_exit_client;
                    Msg_send.socket_id=1;
                    char *buffer1 = new char[sizeof(COMMANT)];
                    Msg_send.msg = buffer1;
                    Msg_send.size=sizeof(COMMANT);
                    COMMANT commant1;
                    commant1.size=sizeof(COMMANT);
                    commant1.com=(unsigned int)socket_command::exceed_max_device;
                    commant1.socket_id=1;
                    memcpy(buffer1,&commant1,sizeof(COMMANT));
                    q_client_msg->push(Msg_send);
                }
            }
            else
            {
                mapUsr->insert(std::pair<unsigned int ,unsigned int >(*usr_id,1));
            }
        }
        log(1);
    }
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
                DGDBG("socket_command: %x",com.com);
                break;
            }
    }
}
