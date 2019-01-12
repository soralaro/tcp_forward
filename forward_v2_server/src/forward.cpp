//
// Created by czx on 18-12-16.
//

#include"omp.h"
#include "../include/forward.h"

std::vector<forward *> forward::forward_Pool;
unsigned  char forward::encryp_key=0xA5;
void forward::forward_pool_int(int max_num,struct sockaddr_in addr)
{
    forward_Pool.resize(max_num);
    for(int i=0;i<max_num;i++) {
        forward_Pool[i] = new forward(addr);
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
forward::forward(struct sockaddr_in addr) {

    end_=true;
    free=true;
    destroy=false;
    id=0;
    servaddr=addr;

    ThreadPool::pool_add_worker(server_rcv, this);
}
void forward::init(unsigned int g_id,BlockQueue<MSG> *q_msg) {
    id=g_id;
    free=false;
    end_=false;
    server_rcv_end=true;
    q_client_msg=q_msg;
    server_socket = socket(AF_INET,SOCK_STREAM, 0);
    std::unique_lock<std::mutex> mlock(mutex_server_socket);
    mlock.unlock();
    cond_server_socket.notify_all();
}
void forward::release()
{
    DGDBG("id=%d forward release start !\n",id);
    end_=true;
    while (server_rcv_end)
    {
        usleep(1000);
    }

    close(server_socket);
    server_socket=-1;
    DGDBG("id=%d forward release end !\n",id);
    id=0;
    free=true;
    q_client_msg=NULL;
}
forward::~forward() {

    DGDBG("id=%d,~forward\n",id);
}

void forward::server_rcv(void *arg) {
    forward *this_class = (forward *)arg;
    this_class->server_socket=false;
    DGDBG("id=%d client_rcv star\n",this_class->id);
    std::unique_lock<std::mutex> mlock(this_class->mutex_server_socket);
    while (!this_class->destroy) {
        this_class->cond_server_socket.wait(mlock);
        if (!this_class->connect_state) {
            if (!this_class->server_connect()) {
                this_class->end_=true;
                this_class->release();
                return;
            }
        }
        while (!this_class->end_) {
            char *buffer = new char[BUFFER_SIZE];
            int len = recv(this_class->server_socket, buffer+sizeof(COMMANT), BUFFER_SIZE-sizeof(COMMANT), 0);
            if (len > 0) {
                // DGDBG("client recv len%d\n", len);
                data_cover((unsigned char *) buffer, len);

                MSG Msg;
                Msg.type = MSG_TPY::msg_client_rcv;
                Msg.from=this_class;
                Msg.msg = buffer;
                COMMANT *commant=(COMMANT *) buffer;
                commant->size=sizeof(COMMANT)+len;
                commant->com=(unsigned int)socket_command::Data;
                commant->socket_id=this_class->id;
                this_class->q_client_msg->push(Msg);
            }
            else {
                delete[] buffer;
                DGDBG("id =%d client recv erro \n", this_class->id);

                struct tcp_info info;

                int info_len=sizeof(info);

                getsockopt(this_class->server_socket, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&info_len);
                if(info.tcpi_state!=TCP_ESTABLISHED)
                {
                   // DGDBG("id =%d tcpi_state!=TCP_ESTABLISHED) \n",this_class->id);
                    break;
                }
                usleep(1000);
            }
        }
        this_class->end_=true;
        MSG Msg;

        Msg.type = MSG_TPY::msg_client_rcv;
        char *buffer = new char[BUFFER_SIZE];
        Msg.from=this_class;
        Msg.msg = buffer;
        COMMANT *commant=(COMMANT *) buffer;
        commant->size=sizeof(COMMANT);
        commant->com=(unsigned int)socket_command::dst_connetc;
        commant->socket_id=this_class->id;
        this_class->q_client_msg->push(Msg);

        DGDBG("id=%d client_rcv exit!\n",this_class->id);
        this_class->cond_server_socket=true;
        this_class->release();
    }


}

int forward::send_all(char *buf,int size)
{
    int ret;
    if(end_)
        return -1;
    int remain=size;
    int sendedSize=0;
    while(remain>0) {
        ret = send(server_socket, buf + sendedSize, remain, 0);
        if(ret>0)
        {
            sendedSize+=ret;
            remain-=ret;
        } else
        {
            //close(client_socket);
            end_=true;
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
        connect_state=true;
        std::unique_lock<std::mutex> mlock(mutex_connect);
        mlock.unlock();
        cond_connect.notify_all();


    }

    DGDBG("id =%d server_connect exit \n",id);
    return true;
}



