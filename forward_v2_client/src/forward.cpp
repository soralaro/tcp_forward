//
// Created by czx on 18-12-16.
//

#include"omp.h"
#include "../include/forward.h"

std::vector<forward *> forward::forward_Pool;
unsigned  char forward::encryp_key=0xA5;
void forward::forward_pool_int(int max_num)
{
    forward_Pool.resize(max_num);
    for(int i=0;i<max_num;i++) {
        forward_Pool[i] = new forward;
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
forward::forward() {

    end_=true;
    free=true;
    destroy=false;
    id=0;
    client_rcv_end=true;
    client_forward_end=true;

    mServer=NULL;
    client_socket=-1;
    ThreadPool::pool_add_worker(client_rcv, this);
    ThreadPool::pool_add_worker(client_forward, this);
}
void forward::init(unsigned int g_id,int socket_int,server *pServer) {
    id=g_id;
    free=false;
    client_socket=socket_int;
    end_=false;
    client_rcv_end=true;
    client_forward_end=true;
    mServer=pServer;
    std::unique_lock<std::mutex> mlock(mutex_client_socket);
    mlock.unlock();
    cond_client_socket.notify_all();
}
void forward::release()
{
    DGDBG("id=%d forward release start !\n",id);
    end_=true;


    while(!q_server_msg.empty())
    {
        MSG Msg;
        q_server_msg.pop(Msg);
        if(Msg.msg!=NULL) {
            char *buf = (char *) Msg.msg;
            delete[] buf;
        }
    }
    close(client_socket);
    client_socket=-1;
    sem_close(&sem_end_);
    DGDBG("id=%d forward release end !\n",id);
    id=0;
    free=true;
    cond_free.notify_all();
}
forward::~forward() {

    DGDBG("id=%d,~forward\n",id);
    sem_close(&sem_end_);
}

void forward::client_rcv(void *arg) {
    forward *this_class = (forward *)arg;
    this_class->client_rcv_end=false;
    DGDBG("id=%d client_rcv star\n",this_class->id);
    std::unique_lock<std::mutex> mlock(this_class->mutex_client_socket);
    while (!this_class->destroy) {
        this_class->cond_client_socket.wait(mlock);
        while (!this_class->free) {
            std::unique_lock<std::mutex> mlock_free(this_class->mutex_free);
            this_class->cond_free.wait(mlock_free);
            while (!this_class->end_) {
                char *buffer = new char[BUFFER_SIZE];
                int len = recv(this_class->client_socket, buffer+sizeof(COMMANT), BUFFER_SIZE-sizeof(COMMANT), 0);
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
                    this_class->mServer->q_client_msg.push(Msg);
                }
                else {
                    delete[] buffer;
                    DGDBG("id =%d client recv erro \n", this_class->id);

                    struct tcp_info info;

                    int info_len=sizeof(info);

                    getsockopt(this_class->client_socket, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&info_len);
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
            Msg.type = MSG_TPY::msg_socket_end;
            Msg.msg = NULL;
            Msg.size = 0;
            this_class->q_server_msg.push(Msg);

            Msg.type = MSG_TPY::msg_client_rcv;
            char *buffer = new char[BUFFER_SIZE];
            Msg.from=this_class;
            Msg.msg = buffer;
            COMMANT *commant=(COMMANT *) buffer;
            commant->size=sizeof(COMMANT);
            commant->com=(unsigned int)socket_command::dst_connetc;
            commant->socket_id=this_class->id;
            this_class->mServer->q_client_msg.push(Msg);

            while(!(this_class->client_forward_end)) {
                sem_wait(&this_class->sem_end_);
            }
            DGDBG("id=%d client_rcv exit!\n",this_class->id);
            this_class->client_rcv_end=true;
            this_class->release();
        }
    }


}



void  forward::client_forward(void *arg) {
    forward *this_class = (forward *)arg;
    this_class->client_forward_end=false;
    signal(SIGPIPE, SIG_IGN);
    DGDBG("id=%d client_forward start! \n",this_class->id);
    std::unique_lock<std::mutex> mlock(this_class->mutex_client_socket);
    while (!this_class->destroy) {
        this_class->cond_client_socket.wait(mlock);
        while (!this_class->free) {
            std::unique_lock<std::mutex> mlock_free(this_class->mutex_free);
            this_class->cond_free.wait(mlock_free);
            while (!this_class->end_) {
                MSG Msg;
                this_class->q_server_msg.pop(Msg);
                if (Msg.type == MSG_TPY::msg_socket_end) {
                    break;
                }
                char *buf = (char *) Msg.msg;
                int ret = send_all(this_class->client_socket, buf, Msg.size);
                delete[] buf;
                if (ret < 0) {
                    close(this_class->client_socket);
                      DGDBG("id =%d client forward send_all <0,close ,server_socket,client_socket\n",this_class->id);
                    break;
                } else {
                    DGDBG("client_forward len=%d\n",Msg.size);
                }
            }
            this_class->end_=true;
            DGDBG("id=%d client_forward exit! \n",this_class->id);
            this_class->client_forward_end=true;
            sem_post(&this_class->sem_end_);
        }
    }

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



