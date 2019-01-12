//
// Created by czx on 18-12-16.
//

#include"omp.h"
#include "../include/server.h"
#include "../include/forward.h"



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
void server::server_pool_int(int max_num)
{
    server_Pool.resize(max_num);
    for(int i=0;i<max_num;i++) {
        server_Pool[i] = new server;
    }
}
server * server::server_pool_get()
{
    for(unsigned int i=0;i<server_Pool.size();i++) {
        if(server_Pool[i]->free)
        {
            return server_Pool[i];
        }
    }
    return NULL;
}
void server::server_pool_destroy()
{
    for(unsigned int i=0;i<server_Pool.size();i++) {
        server_Pool[i]->release();
        delete server_Pool[i];
    }
}
server::server()
{
    end_=false;
    id=0;
    destroy=false;
    client_socket=0;
    forward_end=true;
    rcv_end=true;
    ThreadPool::pool_add_worker(server_rcv, this);
    commandProcess=new command_process(&q_client_msg);
}
server::~server()
{
    delete commandProcess;
}
void server::init(unsigned int g_id,int socket_int,std::string ip ,int port) {

    client_socket=socket_int;
    id=g_id;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);  ///服务器端口
    servaddr.sin_addr.s_addr = inet_addr(ip.c_str());  ///服务器ip
    std::unique_lock<std::mutex> mlock(mutex_client_socket);
    mlock.unlock();
    cond_client_socket.notify_all();

}
void server::release()
{
    close(client_socket);
    if(!forward_end)
    {
        std::unique_lock<std::mutex> mlock(mutex_forward_end);
        cond_forward_end.wait(mlock);
    }
    id=0;
    commandProcess->relase();
    DGDBG("id=%d release end !\n",id);
    free=true;
}



void server::server_rcv(void *arg) {

    server *this_class = (server *)arg;
    DGDBG("id=%d server_rcv star! \n",this_class->id);
    std::unique_lock<std::mutex> mlock(this_class->mutex_client_socket);
    while (!this_class->destroy) {
        this_class->cond_client_socket.wait(mlock);
        this_class->rcv_end=false;
        while (!this_class->end_) {
            char *buffer=new char[BUFFER_SIZE];

            int len = recv(this_class->client_socket, buffer, BUFFER_SIZE, 0);
            if (len > 0) {
                // DGDBG("server recv len%d\n", len);
                data_cover((unsigned char *) buffer, len);
                this_class->commandProcess.process((unsigned char *)buffer, len);
            } else {
                struct tcp_info info;

                int info_len = sizeof(info);

                getsockopt(this_class->client_socket, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *) &info_len);
                if (info.tcpi_state != TCP_ESTABLISHED) {
                    // DGDBG("id =%d tcpi_state!=TCP_ESTABLISHED) \n",this_class->id);
                    this_class->end_=true;
                    break;
                }
                usleep(1000);
            }
        }
        this_class->rcv_end=true;
        this_class->release();
    }
    DGDBG("id =%d server_rcv exit \n",this_class->id);
}

void  server::forward(void *arg) {
    server *this_class = (server *)arg;
    DGDBG("id=%d server_forward star! \n",this_class->id);
    std::unique_lock<std::mutex> mlock(this_class->mutex_client_socket);
    while (!this_class->destroy) {
        this_class->cond_client_socket.wait(mlock);
        this_class->server_forward_end = false;
        signal(SIGPIPE, SIG_IGN);
        DGDBG("id =%d server_forward start \n", this_class->id);
        while (!this_class->end_) {
            MSG Msg;
            this_class->q_client_msg.pop(Msg);
            if (Msg.type == MSG_TPY::msg_socket_end) {
                break;
            }
            if(Msg.type==msg_socket_end)
            {
                this_class->commandProcess->erease_mforwar(((forward *)Msg.from)->id);
            }
            char *buf = (char *) Msg.msg;
            int ret = this_class->send_all(buf, Msg.size);
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
        this_class->forward_end=true;
        if(this_class->rcv_end) {
            std::unique_lock<std::mutex> mlock(this_class->mutex_forward_end);
            mlock.unlock();
            this_class->cond_forward_end.notify_all();
        }
    }

    DGDBG("id =%d server_forward exit \n",this_class->id);

}

int server::send_all( char *buf,int size)
{
    int ret;
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
            end_=true;
            return -1;
        }
    }
    return 1;
}


