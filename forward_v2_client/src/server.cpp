//
// Created by czx on 18-12-16.
//

#include"omp.h"
#include "../include/server.h"


unsigned char server::encryp_key=0x55;
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
    connect_state=false;
    server_socket=0;
    memset(&servaddr,0,sizeof(servaddr));
    commandProcess=new command_process(&q_client_msg);
    commandProcess->encry_data=encry_data;
    commandProcess->get_encrypt_state=&get_encrypt_state;
}
server::~server() {
    delete commandProcess;

}
void server::init(std::string ip ,int port) {

    get_encrypt_state=false;
    end_=false;
    forward_end=true;
    connect_state=false;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);  ///服务器端口
    servaddr.sin_addr.s_addr = inet_addr(ip.c_str());  ///服务器ip
    server_socket = socket(AF_INET,SOCK_STREAM, 0);
    heart_beat=0;
    ThreadPool::pool_add_worker(server_rcv, this);
    ThreadPool::pool_add_worker(server_forward, this);
    ThreadPool::pool_add_worker(timer_fuc, this);

}
bool server::add_forward(unsigned int g_id, int socket_int) {
    if(!connect_state)
    {
        return false;
    }
    if(!get_encrypt_state)
    {
        return false;
    }
    forward *forward = forward::forward_pool_get();
    if(forward!=NULL) {
        static unsigned  int id=0;
        forward->init(g_id,socket_int,&q_client_msg);
        commandProcess->add_mforward(forward);
        //printf("new connect id=%d \n",forward->id);
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
        MSG msg;
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
        MSG Msg;
        q_client_msg.pop(Msg);
        char *buf = (char *) Msg.msg;
        delete[] buf;
    }
    close(server_socket);
    server_socket = socket(AF_INET,SOCK_STREAM, 0);
    send_sn=0;
    heart_beat=0;
    DGDBG("id=%d release end !\n",id);
}

void server::timer_fuc(void *arg)
{

    server *this_class = (server *)arg;
    while(!this_class->end_) {
        sleep(3);
        if(this_class->connect_state) {
            this_class->heart_beat++;
            if (this_class->heart_beat > 10) {
                this_class->connect_state = false;
                this_class->heart_beat=0;
            } else
            {
                MSG Msg;
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
        }
    }
}
void  server::server_forward(void *arg) {
    server *this_class = (server *)arg;
    signal(SIGPIPE, SIG_IGN);
    std::unique_lock<std::mutex> mlock(this_class->mutex_connect);
    DGDBG("id =%d server_forward start \n",this_class->id);
    while (!this_class->end_) {
        while(!this_class->connect_state)
        {
            this_class->forward_end=true;
            this_class->cond_connect.wait(mlock);
        }
        this_class->forward_end= false;
        MSG Msg;
        Msg.size=0;
        this_class->q_client_msg.pop(Msg);
        if( Msg.type==MSG_TPY::msg_server_release)
        {
            continue;
        }
        if( Msg.type==MSG_TPY::msg_socket_end) {
            this_class->commandProcess->erease_mforward(Msg.socket_id);
        }

        if(Msg.size>0) {
            char *buf=(char *)Msg.msg;
            COMMANT commant;
            memcpy(&commant, buf, sizeof(commant));
            commant.sn = this_class->send_sn++;
            DGDBG("server_forward_commant size=%x,sn=%x,id=%x,com=%x ", commant.size, commant.sn, commant.socket_id,
                  commant.com);
            memcpy(buf, &commant, sizeof(commant));
            this_class->data_encrypt((unsigned char *) buf, Msg.size);

            int ret = send_all(this_class->server_socket, buf, Msg.size);
            delete[] buf;
            if (ret < 0) {
                DGDBG("id =%d send <0,server_forward\n", this_class->id);
                this_class->connect_state = false;
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
    while (!this_class->end_)
    {
        if(!this_class->connect_state)
        {
            this_class->release();
            if(!this_class->server_connect())
            {
                sleep(1);
                continue;
            }
        }
        static char buffer[BUFFER_SIZE];
        DGDBG("id=%d server waiting rcv! \n",this_class->id);
        int len = recv(this_class->server_socket, buffer,BUFFER_SIZE, 0);
        if (len > 0) {
             DGDBG("server recv len%d\n", len);
             if(!this_class->get_encrypt_state)
                data_cover((unsigned char *)buffer, len);
            this_class->commandProcess->process((unsigned char*)buffer,len);
            this_class->heart_beat=0;
        }
        else
        {
            struct tcp_info info;

            int info_len=sizeof(info);

            getsockopt(this_class->server_socket, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&info_len);
            if(info.tcpi_state!=TCP_ESTABLISHED)
            {
               DGDBG("id =%d tcpi_state!=TCP_ESTABLISHED) \n",this_class->id);
               this_class->connect_state=false;
            }
            usleep(1000);
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
    DGDBG("id=%d server_connect star! \n",id);
    if (connect(server_socket, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        // perror("server connect");
        close(server_socket);
        DGDBG("server connect fail ,id=%d",id);
        return false;
    }
    else {
#if 1
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
        DGDBG("id =%d ,server connect suc \n",id);
        connect_state=true;
        send_sn=0;
        std::unique_lock<std::mutex> mlock(mutex_connect);
        mlock.unlock();
        cond_connect.notify_all();
    }
    DGDBG("id =%d server_connect exit \n",id);
    return true;
}

