//
// Created by czx on 18-12-16.
//

#include"omp.h"
#include "../include/server.h"
#include "encrypt.h"


unsigned char server::encryp_key=0x55;
unsigned char server::encryp_key_2=0xae;
unsigned int  server::user_id=1;
char server::des_key[17];
char server::des_key_2[17];
char server::des_key_3[17];
void server::setKey(unsigned  char input_key)
{
    encryp_key=input_key;
}

void server::data_cover(unsigned char *buf, int len)
{
    //#pragma omp parallel for num_threads(4)
    for(int i=0;i<len;i++)
    {
        buf[i]=(buf[i]^encryp_key);
    }
}

void server::data_encrypt(unsigned char *buf, int len)
{

    if(encry_data==NULL)
    {
        return;
    }
    for(int i=0;i<len;i++)
    {
        buf[i]=(buf[i]^encry_data[i]);
    }
}

server::server()
{
    end_=true;
    forward_end=true;
    get_encrypt_state=false;
    id=0;
    connect_lock.lock();
    connect_state=DISCONNECT;
    connect_lock.unlock();
    server_socket=0;
    memset(&servaddr,0,sizeof(servaddr));
    commandProcess=new command_process(&q_client_msg);
    commandProcess->encry_data=encry_data;
    commandProcess->get_encrypt_state=&get_encrypt_state;
    idel_time_set(0);
}
server::~server() {
    delete commandProcess;

}
void server::heart_beat_set(int value)
{
    heart_beat_mutex.lock();
    if(value==0)
        heart_beat=0;
    else
        heart_beat+=value;
    heart_beat_mutex.unlock();
}
int server::heart_beat_get()
{
    int value;
    heart_beat_mutex.lock();
    value=heart_beat;
    heart_beat_mutex.unlock();
    return value;
}
void server::idel_time_set(int value){
    idel_time_mutex.lock();
    if(value==0)
        idel_time=0;
    else
        idel_time+=value;
    idel_time_mutex.unlock();
}
int server::idel_time_get()
{
    int value;
    idel_time_mutex.lock();
    value=idel_time;
    idel_time_mutex.unlock();
    return value;
}
void server::init(std::string ip ,int port) {

    srand((int)time(0));
    get_encrypt_state=false;
    end_=false;
    forward_end=true;
    connect_lock.lock();
    connect_state=DISCONNECT;
    connect_lock.unlock();
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);  ///服务器端口
    servaddr.sin_addr.s_addr = inet_addr(ip.c_str());  ///服务器ip
    server_socket = socket(AF_INET,SOCK_STREAM, 0);
    heart_beat_set(0);
    idel_time_set(0);
    commandProcess->encryp_key_2=encryp_key_2;
    commandProcess->encryp_key=encryp_key;
    des_encrypt_init(des_key);
    des_encrypt_init_2(des_key_2);
    des_encrypt_init_3(des_key_3);
    ThreadPool::pool_add_worker(server_rcv, this);
    ThreadPool::pool_add_worker(server_forward, this);
    ThreadPool::pool_add_worker(timer_fuc, this);

}
bool server::add_forward(unsigned int g_id, int socket_int) {
    idel_time_set(0);
    connect_lock.lock();
    if(connect_state!=CONNECTED)
    {
        if(connect_state==CONNECT_WAIT) {
            connect_lock.unlock();
            std::unique_lock<std::mutex> mlock(mutex_notic_connect);
            mlock.unlock();
            cond_notic_connect.notify_all();
        } else
        {
            connect_lock.unlock();
        }
        return false;
    } else {
        connect_lock.unlock();
    }
    if(!get_encrypt_state)
    {
        return false;
    }
    forward *forward = forward::forward_pool_get();
    if(forward!=NULL) {
        forward->init(g_id,socket_int,&q_client_msg);
        commandProcess->add_mforward(forward);
    } else
    {
        printf("forward_pool_get false! \n");
        return false;
    }
    return  true;
}
void server::release()
{
    if(!forward_end)
    {
        MSG_COM msg;
        msg.type=MSG_TPY::msg_server_release;
        msg.size=0;
        msg.msg=NULL;
        q_client_msg.push(msg);
        while (!forward_end)
        {
            sleep(1);
        }
    }
    get_encrypt_state=false;

    commandProcess->relase();
    while (!q_client_msg.empty()) {
        MSG_COM Msg;
        q_client_msg.pop(Msg);
        char *buf = (char *) Msg.msg;
        delete[] buf;
    }
    close(server_socket);
    server_socket = socket(AF_INET,SOCK_STREAM, 0);
    send_sn=0;
    heart_beat_set(0);
    DGDBG("id=%d release end !\n",id);
    printf("id=%d disconnect server!\n",id);
}

void server::timer_fuc(void *arg)
{

    server *this_class = (server *)arg;
    while(!this_class->end_) {
        sleep(3);
        if(this_class->commandProcess->get_mforward_size()>0)
        {
            this_class->idel_time_set(0);
        } else {
            this_class->idel_time_set(1);
        }
        this_class->heart_beat_set(1);
        this_class->connect_lock.lock();
        if(this_class->connect_state==CONNECTED) {
            this_class->connect_lock.unlock();
            if (this_class->heart_beat_get()> 10) {
                this_class->connect_lock.lock();
                this_class->connect_state = DISCONNECT;
                this_class->connect_lock.unlock();
                this_class->heart_beat_set(0);
                DGERR("heart_beat time out!");
            } else if(this_class->idel_time_get()<IDEL_TIME_MAX)
            {
                MSG_COM Msg;
                Msg.socket_id=2;
                Msg.size=sizeof(COMMANT);
                Msg.type=MSG_TPY::msg_heart_beat;
                COMMANT commant;
                commant.size=sizeof(COMMANT);
                commant.socket_id=2;
                commant.com=(unsigned int)socket_command::heart_beat;
                unsigned  char *buf=new unsigned char [sizeof(COMMANT)];
                memcpy(buf,&commant,sizeof(commant));
                Msg.msg=buf;
                this_class->q_client_msg.push(Msg);
            }
        } else {
            this_class->connect_lock.unlock();
        }
    }
}
void  server::server_forward(void *arg) {
    server *this_class = (server *)arg;
    unsigned  char ex_buf[128];
#ifndef  _WIN64    
    signal(SIGPIPE, SIG_IGN);
#endif
    std::unique_lock<std::mutex> mlock(this_class->mutex_connect);
    DGDBG("id =%d server_forward start \n",this_class->id);
    while (!this_class->end_) {
        this_class->connect_lock.lock();
        while(this_class->connect_state!=CONNECTED)
        {
            this_class->connect_lock.unlock();
            this_class->forward_end=true;
            this_class->cond_connect.wait(mlock);
            this_class->connect_lock.lock();
        }
        this_class->connect_lock.unlock();
        this_class->forward_end= false;
        MSG_COM Msg;
        Msg.size=0;
        this_class->q_client_msg.pop(Msg);
        if( Msg.type==MSG_TPY::msg_server_release)
        {
            if(Msg.size>0)
            {
                char *buf=(char *)Msg.msg;
                delete[] buf;
            }
            continue;
        }
        if( Msg.type==MSG_TPY::msg_socket_end) {
            this_class->commandProcess->erease_mforward(Msg.socket_id);
        }

        if(Msg.type==MSG_TPY::msg_client_rcv)
        {
            if(!this_class->commandProcess->check_mforward_exist(Msg.socket_id))
            {
                if(Msg.size>0)
                {
                    char *buf=(char *)Msg.msg;
                    delete[] buf;
                }
                continue;
            }
        }

        if(Msg.size>0) {
            char *buf=(char *)Msg.msg;
            COMMANT commant;
            memcpy(&commant, buf, sizeof(commant));
            commant.sn = this_class->send_sn++;
            commant.res0 =rand();
            commant.ex_size=rand();
            commant.user_id=user_id;
            DGDBG("server_forward_commant size=%x,sn=%x,id=%x,com=%x ", commant.size, commant.sn, commant.socket_id,
                  commant.com);
            memcpy(buf, &commant, sizeof(commant));
            this_class->data_encrypt((unsigned char *) buf, Msg.size);
            des_encrypt((unsigned char *)buf,sizeof(commant));

          
            des_encrypt_3((unsigned char *)buf,sizeof(commant));

            des_encrypt_2((unsigned char *)buf+sizeof(commant),ALIGN_16(Msg.size-sizeof(commant)));
            des_encrypt_3((unsigned char *)buf+sizeof(commant),ALIGN_16(Msg.size-sizeof(commant)));
            int ret = send_all(this_class->server_socket, buf, ALIGN_16(Msg.size));
            if(ret>0)
            {
                int ex_len=0x7f&commant.ex_size;
                for(int i=0;i<ex_len;i++)
                {
                    ex_buf[i]=rand();
                }
                ret = send_all(this_class->server_socket, (char *)ex_buf, ex_len);
            }

            delete[] buf;
            if (ret < 0) {
                DGDBG("id =%d send <0,server_forward\n", this_class->id);

                this_class->connect_lock.lock();
                if(this_class->connect_state==CONNECTED)
                    this_class->connect_state=DISCONNECT;
                this_class->connect_lock.unlock();
            } else {
                DGDBG("server_forwar =%d\n", Msg.size);
            }
        }

    }
    DGDBG("id =%d server_forward exit \n",this_class->id);
}

void server::server_rcv(void *arg) {

    server *this_class = (server *)arg;
    DGDBG("id=%d server_rcv star! \n",this_class->id);
    std::unique_lock<std::mutex> mlock(this_class->mutex_notic_connect);
    while (!this_class->end_)
    {
        this_class->connect_lock.lock();
        if(this_class->connect_state==DISCONNECT)
        {
            this_class->connect_lock.unlock();
            this_class->release();
            if(this_class->idel_time_get()>IDEL_TIME_MAX) {
                this_class->connect_lock.lock();
                this_class->connect_state=CONNECT_WAIT;
                this_class->connect_lock.unlock();
                this_class->cond_notic_connect.wait(mlock);
            }
            this_class->connect_lock.lock();
            this_class->connect_state=CONNECTING;
            this_class->connect_lock.unlock();
            if(!this_class->server_connect())
            {
                sleep(1);
                continue;
            }
        } else {
            this_class->connect_lock.unlock();
        }
        static char buffer[BUFFER_SIZE+1];
        DGDBG("id=%d server waiting rcv! \n",this_class->id);
        int len = recv(this_class->server_socket, buffer,BUFFER_SIZE, 0);
        if (len > 0) {
             DGDBG("server recv len%d\n", len);

            this_class->commandProcess->process((unsigned char*)buffer,len);
            this_class->heart_beat_set(0);
        }
        else
        {
#ifdef _WIN64 
            //DGDBG("id =%d client recv erro \n",this_class->id);
            if(errno == EAGAIN||errno == EWOULDBLOCK||errno == EINTR)
            {
                usleep(1000);
                DGDBG("id =%d client_rcv erro=%d \n",this_class->id,errno);
            } 
            else
            {
                DGDBG("id =%d tcpi_state!=TCP_ESTABLISHED) \n",this_class->id);
                this_class->connect_lock.lock();
                if(this_class->connect_state==CONECTED)
                    this_class->connect_state=0;
                this_class->connect_lock.unlock();
            }
#else
            struct tcp_info info;

            int info_len=sizeof(info);

            getsockopt(this_class->server_socket, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&info_len);
            if(info.tcpi_state!=TCP_ESTABLISHED)
            {
               DGDBG("id =%d tcpi_state!=TCP_ESTABLISHED) \n",this_class->id);
               this_class->connect_lock.lock();
               if(this_class->connect_state==CONNECTED)
                    this_class->connect_state=DISCONNECT;
               this_class->connect_lock.unlock();
            }
            usleep(1000);
#endif
        }
    }
    DGDBG("id =%d server_rcv exit \n",this_class->id);
}



int server::send_all(int socket, char *buf,int size)
{
    int ret;
    int remain=size;
    int sendedSize=0;
    while(remain>0) {
        ret = send(socket, buf + sendedSize, remain, 0);
        if(ret>0)
        {
            sendedSize+=ret;
            remain-=ret;
        } else
        {
            return -1;
        }
    }
    return 1;
}

bool server::server_connect()
{
    printf("id=%d server_connect star! \n",id);

    if (connect(server_socket, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("server connect");
        close(server_socket);
        DGERR("server connect fail ,id=%d",id);
        connect_lock.lock();
        connect_state=DISCONNECT;
        connect_lock.unlock();
        return false;
    }
    else {
#ifdef _WIN64
        int timeout = 12000;//3s
        int ret = setsockopt(server_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
        if (ret < 0) {
            perror("setsockopt SO_SNDTIMEO");
            exit(1);
        }
        ret = setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
        if (ret < 0) {
            perror("setsockopt SO_RCVTIMEO");
            exit(1);
        }
#else
        struct timeval timeout = {6, 0};//3s
        int ret = setsockopt(server_socket, SOL_SOCKET, SO_SNDTIMEO,  &timeout, sizeof(timeout));
        if (ret < 0) {
            perror("setsockopt SO_SNDTIMEO");
            exit(1);
        }
        ret = setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        if (ret < 0) {
            perror("setsockopt SO_RCVTIMEO");
            exit(1);
        }

        socklen_t optlen = sizeof(struct timeval);

        struct timeval tv;

        memset(&tv,0,sizeof(tv));

        getsockopt(server_socket, SOL_SOCKET,SO_RCVTIMEO, &tv, &optlen);
        DGDBG("id=%d server tv.tv_sec=%ld,tv_usec=%ld \n",id,tv.tv_sec,tv.tv_usec);

#endif
        printf("id =%d ,server connect suc \n",id);

        send_sn=0;
        heart_beat_set(0);
        std::unique_lock<std::mutex> mlock(mutex_connect);
        connect_lock.lock();
        connect_state=CONNECTED;
        connect_lock.unlock();
        mlock.unlock();
        cond_connect.notify_all();
    }

    DGDBG("id =%d server_connect exit \n",id);
    return true;
}

