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
    free=true;
    id=0;
    DGDBG("id=%d release end !\n",id);
}



void server::server_rcv(void *arg) {

    server *this_class = (server *)arg;
    DGDBG("id=%d server_rcv star! \n",this_class->id);
    std::unique_lock<std::mutex> mlock(this_class->mutex_client_socket);
    while (!this_class->destroy) {
        this_class->cond_client_socket.wait(mlock);

        while (!this_class->end_) {
            static char buffer[BUFFER_SIZE];

            int len = recv(this_class->client_socket, buffer, BUFFER_SIZE, 0);
            if (len > 0) {
                // DGDBG("server recv len%d\n", len);
                data_cover((unsigned char *) buffer, len);
                this_class->commandProcess.process(buffer, len);
            } else {
                struct tcp_info info;

                int info_len = sizeof(info);

                getsockopt(this_class->client_socket, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *) &info_len);
                if (info.tcpi_state != TCP_ESTABLISHED) {
                    // DGDBG("id =%d tcpi_state!=TCP_ESTABLISHED) \n",this_class->id);
                    close(this_class->client_socket);
                    this_class->end_=true;
                    break;
                }
                usleep(1000);
            }
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



