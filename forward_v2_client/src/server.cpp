//
// Created by czx on 18-12-16.
//

#include"omp.h"
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
server::server(int g_id) {


}
void server::init(int socket_int,std::string ip ,int port) {

    client_socket=socket_int;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);  ///服务器端口
    servaddr.sin_addr.s_addr = inet_addr(ip.c_str());  ///服务器ip
    server_socket = socket(AF_INET,SOCK_STREAM, 0);
    threadPool::pool_add_worker(server_rcv, this);
    threadPool::pool_add_worker(server_forward, this);

}
void server::release()
{
    close(server_socket);
    DGDBG("id=%d release end !\n",id);
}
server::~server() {

    DGDBG("id=%d,~ard\n",id);
}


void  server::server_forward(void *arg) {
    forward *this_class = (forward *)arg;
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

void server::server_rcv(void *arg) {

    forward *this_class = (forward *)arg;
    DGDBG("id=%d server_rcv star! \n",this_class->id);
    while (!this_class->end_)
    {
        if(!connect_state)
        {
            if(!server_connect())
            {
                sleep(1);
                continue;
            }
        }
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
        else
        {
            struct tcp_info info;

            int info_len=sizeof(info);

            getsockopt(this_class->server_socket, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&info_len);
            if(info.tcpi_state!=TCP_ESTABLISHED)
            {
               // DGDBG("id =%d tcpi_state!=TCP_ESTABLISHED) \n",this_class->id);
                close(this_class->server_socket);
                connect_state=false;
            }
            usleep(1000);
        }
    }
    DGDBG("id =%d server_rcv exit \n",this_class->id);
    sem_post(&this_class->sem_end_);
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
    }
    DGDBG("id =%d server_connect exit \n",id);
    return true;
}

