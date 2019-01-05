//
// Created by czx on 18-12-16.
//

#include"omp.h"
#include "../include/forward.h"

ThreadPool  forward::threadPool;
std::vector<forward *> forward::forward_Pool;
unsigned  char forward::encryp_key=0xA5;
void forward::forward_pool_int(int max_num)
{
    forward_Pool.resize(max_num);
    for(int i=0;i<max_num;i++) {
        forward_Pool[i] = new forward(i);
    }
}
forward * forward::forward_pool_get()
{
    for(unsigned int i=0;i<forward_Pool.size();i++) {
        if(forward_Pool[i]->free)
        {
            return forward_Pool[i];
        }
    }
    return NULL;
}

void forward::setKey(unsigned  char input_key)
{
    encryp_key=input_key;
}
void forward::forward_pool_destroy()
{
    for(unsigned int i=0;i<forward_Pool.size();i++) {
        forward_Pool[i]->release();
        delete forward_Pool[i];
    }
}
void forward::data_cover(unsigned char *buf, int len)
{
    //#pragma omp parallel for num_threads(4)
    for(int i=0;i<len;i++)
    {
        buf[i]=(buf[i]^encryp_key);
    }
}
forward::forward(int g_id) {

    end_=false;
    id=g_id;
    client_rcv_end=true;
    client_forward_end=true;
    server_rcv_end=true;
    server_forward_end=true;
    free=true;
}
void forward::init(int socket_int,std::string ip ,int port) {

    free=false;
    client_socket=socket_int;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);  ///服务器端口
    servaddr.sin_addr.s_addr = inet_addr(ip.c_str());  ///服务器ip
    server_socket = socket(AF_INET,SOCK_STREAM, 0);
    end_=false;
    client_rcv_end=true;
    client_forward_end=true;
    server_rcv_end=true;
    server_forward_end=true;
    threadPool.pool_add_worker(client_rcv, this);

}
void forward::release()
{
    DGDBG("id=%d forward release start !\n",id);
    end_=true;

    while(!q_client_msg.empty())
    {
        MSG Msg;
        q_client_msg.pop(Msg);
        if(Msg.msg!=NULL) {
            char *buf = (char *) Msg.msg;
            delete[] buf;
        }
    }
    while(!q_server_msg.empty())
    {
        MSG Msg;
        q_server_msg.pop(Msg);
        if(Msg.msg!=NULL) {
            char *buf = (char *) Msg.msg;
            delete[] buf;
        }
    }

    close(server_socket);
    close(client_socket);
    sem_close(&sem_end_);
    DGDBG("id=%d forward release end !\n",id);
    free=true;
}
forward::~forward() {

    DGDBG("id=%d,~forward\n",id);

}

void forward::client_rcv(void *arg) {
    forward *this_class = (forward *)arg;
    this_class->client_rcv_end=false;
    DGDBG("id=%d client_rcv star\n",this_class->id);
    if(this_class->server_connect()==false)
    {
        this_class->end_=true;
        this_class->client_rcv_end=true;
        this_class->release();
        return;
    }
    threadPool.pool_add_worker(client_forward, this_class);
    threadPool.pool_add_worker(server_rcv, this_class);
    threadPool.pool_add_worker(server_forward, this_class);
    while (!this_class->end_)
    {
        char *buffer=new char[BUFFER_SIZE];
        int len = recv(this_class->client_socket, buffer, BUFFER_SIZE, 0);
        if(len>0) {
           // DGDBG("client recv len%d\n", len);
            data_cover((unsigned char *)buffer, len);

            MSG Msg;
            Msg.type=MSG_TPY::msg_client_rcv;
            Msg.msg=buffer;
            Msg.size=len;
            this_class->q_client_msg.push(Msg);
        }
#if 1
        else if(len==0)
        {
            delete[] buffer;
            break;
            // DGDBG("id=%d,client recv time out\n",this_class->id);
        }
#endif
        else
        {
            delete[] buffer;
            DGDBG("id =%d client recv erro \n",this_class->id);
            if(errno == EAGAIN||errno == EWOULDBLOCK||errno == EINTR)
            {
                usleep(1000);
                DGDBG("id =%d client_rcv erro=%d \n",this_class->id,errno);
            } else
            {
                break;
            }
#if 0
            struct tcp_info info;

            int info_len=sizeof(info);

            getsockopt(this_class->client_socket, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&info_len);
            if(info.tcpi_state!=TCP_ESTABLISHED)
            {
               // DGDBG("id =%d tcpi_state!=TCP_ESTABLISHED) \n",this_class->id);
                break;
            }
            usleep(1000);
#endif
        }
    }

    this_class->end_=true;
    MSG Msg;
    Msg.type = MSG_TPY::msg_socket_end;
    Msg.msg = NULL;
    Msg.size = 0;
    this_class->q_server_msg.push(Msg);
    this_class->q_client_msg.push(Msg);
    while(!(this_class->client_forward_end&&this_class->server_rcv_end&&this_class->server_forward_end)) {
        sem_wait(&this_class->sem_end_);
    }
    DGDBG("id=%d client_rcv exit!\n",this_class->id);
    this_class->client_rcv_end=true;
    this_class->release();
}

void  forward::server_forward(void *arg) {
    forward *this_class = (forward *)arg;
    this_class->server_forward_end=false;
    signal(SIGPIPE, SIG_IGN);
    DGDBG("id =%d server_forward start \n",this_class->id);
    while (!this_class->end_) {
        MSG Msg;
        this_class->q_client_msg.pop(Msg);
        if( Msg.type==MSG_TPY::msg_socket_end) {
            break;
        }
        char *buf=(char *)Msg.msg;
        int ret = send_all(this_class->server_socket, buf, Msg.size);
        delete[] buf;
        if (ret < 0) {
            //DGDBG("id =%d send <0,server_forward\n",this_class->id);
            close(this_class->server_socket);
            break;
        } else {
           // DGDBG("server_forwar =%d\n",Msg.size);
        }

    }
    this_class->end_=true;
    DGDBG("id =%d server_forward exit \n",this_class->id);
    this_class->server_forward_end=true;
    sem_post(&this_class->sem_end_);
}

void forward::server_rcv(void *arg) {

    forward *this_class = (forward *)arg;
    this_class->server_rcv_end=false;
    DGDBG("id=%d server_rcv star! \n",this_class->id);
    while (!this_class->end_)
    {
        char *buffer = new char[BUFFER_SIZE];
        int len = recv(this_class->server_socket, buffer,BUFFER_SIZE, 0);
        if (len > 0) {
            // DGDBG("server recv len%d\n", len);
            data_cover((unsigned char *)buffer, len);
            MSG Msg;
            Msg.type = MSG_TPY::msg_server_rcv;
            Msg.msg = buffer;
            Msg.size = len;
            this_class->q_server_msg.push(Msg);
        }
#if 1
        else if(len==0)
        {
            delete[] buffer;
            //usleep(1000);
            break;
            // DGDBG("id=%d,server_rcv,time out\n",this_class->id);
        }
#endif
        else  {
            delete[] buffer;
            usleep(1000);
          //  DGDBG("id =%d server ,rcv len <0\n",this_class->id);
            if(errno == EAGAIN||errno == EWOULDBLOCK||errno == EINTR)
            {
                usleep(1000);
                DGDBG("id =%d server_rcv erro=%d \n",this_class->id,errno);
            } else
            {
               break;
            }
#if 0
            struct tcp_info info;

            int info_len=sizeof(info);
            getsockopt(this_class->server_socket, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&info_len);
            if(info.tcpi_state!=TCP_ESTABLISHED)
            {
               // DGDBG("id =%d tcpi_state!=TCP_ESTABLISHED) \n",this_class->id);
                break;
            }
#endif
        }
    }
    this_class->end_=true;
    DGDBG("id =%d server_rcv exit \n",this_class->id);
    this_class->server_rcv_end=true;
    sem_post(&this_class->sem_end_);
}

void  forward::client_forward(void *arg) {
    forward *this_class = (forward *)arg;
    this_class->client_forward_end=false;
    signal(SIGPIPE, SIG_IGN);
    DGDBG("id=%d client_forward start! \n",this_class->id);
    while (!this_class->end_) {
        MSG Msg;
        this_class->q_server_msg.pop(Msg);
        if( Msg.type==MSG_TPY::msg_socket_end) {
            break;
        }
        char *buf=(char *)Msg.msg;
        int ret = send_all(this_class->client_socket, buf, Msg.size);
        delete[] buf;
        if (ret < 0) {
            close(this_class->client_socket);
          //  DGDBG("id =%d client forward send_all <0,close ,server_socket,client_socket\n",this_class->id);
            break;
        } else
        {
            //DGDBG("client_forward len=%d\n",Msg.size);
        }
    }
    this_class->end_=true;
    DGDBG("id=%d client_forward exit! \n",this_class->id);
    this_class->client_forward_end=true;
    sem_post(&this_class->sem_end_);
}

int forward::send_all(int socket, char *buf,int size)
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

bool forward::server_connect()
{
    DGDBG("id=%d server_connect star! \n",id);
    if (connect(server_socket, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        // perror("server connect");
        close(server_socket);
        close(client_socket);
        return false;
    }
    else {
#if 1
        struct timeval timeout = {1, 0};//3s
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
    }
    DGDBG("id =%d server_connect exit \n",id);
    return true;
}

