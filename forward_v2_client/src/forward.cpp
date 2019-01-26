//
// Created by czx on 18-12-16.
//

#include"omp.h"
#include "../include/forward.h"

std::vector<forward *> forward::forward_Pool;
unsigned  char forward::encryp_key=0xAB;
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

forward::forward() {

    end_=true;
    free=true;
    destroy=false;
    id=0;
    client_rcv_end=true;
    q_client_msg=NULL;
    client_socket=-1;
    ThreadPool::pool_add_worker(client_rcv, this);
}
void forward::init(unsigned int g_id,int socket_int,BlockQueue<MSG> *q_msg) {
    id=g_id;
    free=false;
    client_socket=socket_int;
    end_=false;
    client_rcv_end=true;
    q_client_msg=q_msg;
    std::unique_lock<std::mutex> mlock(mutex_client_socket);
    mlock.unlock();
    cond_client_socket.notify_all();
}
void forward::release()
{
    DGDBG("id=%d forward release start !\n",id);
    end_=true;
    while (!client_rcv_end)
    {
        usleep(1000);
    }

    close(client_socket);
    client_socket=-1;
    DGDBG("id=%d forward release end !\n",id);
    id=0;
    free=true;
}
forward::~forward() {

    DGDBG("id=%d,~forward\n",id);
}

void forward::client_rcv(void *arg) {
    forward *this_class = (forward *)arg;

    DGDBG("id=%d client_rcv star\n",this_class->id);
    std::unique_lock<std::mutex> mlock(this_class->mutex_client_socket);
    while (!this_class->destroy) {
        this_class->cond_client_socket.wait(mlock);
        this_class->client_rcv_end=false;
        while (!this_class->end_) {
            char *buffer = new char[BUFFER_SIZE];
            int len = recv(this_class->client_socket, buffer+sizeof(COMMANT), BUFFER_SIZE-sizeof(COMMANT), 0);
            if (len > 0) {
                DGDBG("client recv len%d\n", len);
                if(this_class->end_)
                {
                    delete [] buffer;
                    break;
                }

                MSG Msg;
                Msg.type = MSG_TPY::msg_client_rcv;
                Msg.socket_id=this_class->id;
                Msg.msg = buffer;
                Msg.size=sizeof(COMMANT)+len;
                COMMANT commant;
                commant.size=sizeof(COMMANT)+len;
                commant.com=(unsigned int)socket_command::Data;
                commant.socket_id=this_class->id;
                memcpy(buffer,&commant,sizeof(commant));
                this_class->q_client_msg->push(Msg);
            }
            else {
                delete[] buffer;
               // DGDBG("id =%d client recv erro \n", this_class->id);

                struct tcp_info info;

                int info_len=sizeof(info);

                getsockopt(this_class->client_socket, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&info_len);
                if(info.tcpi_state!=TCP_ESTABLISHED)
                {
                    DGDBG("id =%d client tcpi_state!=TCP_ESTABLISHED) \n",this_class->id);
                    break;
                }
                usleep(1000);
            }
        }
        this_class->end_=true;
        MSG Msg;

        Msg.type = MSG_TPY::msg_socket_end;
        char *buffer = new char[BUFFER_SIZE];
        Msg.socket_id=this_class->id;
        Msg.msg = buffer;
        Msg.size=sizeof(COMMANT);
        COMMANT commant;
        commant.size=sizeof(COMMANT);
        commant.com=(unsigned int)socket_command::dst_connetc;
        commant.socket_id=this_class->id;
        memcpy(buffer,&commant,sizeof(commant));
        this_class->q_client_msg->push(Msg);

        DGDBG("id=%d client_rcv exit!\n",this_class->id);
        this_class->client_rcv_end=true;
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
        ret = send(client_socket, buf + sendedSize, remain, 0);
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



