//
// Created by czx on 18-12-19.
//

#ifndef PROJECT_FORWAR_SERVER_H
#define PROJECT_FORWAR_SERVER_H
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <thread>
#include <iostream>
#include <queue>
#include <semaphore.h>
#include <signal.h>
#include <netinet/tcp.h>
#include <map>

#define BUFFER_SIZE 1024

 enum class MSG_TPY
{
    msg_client_rcv=0,
    msg_server_rcv
};

typedef struct MSG_struct
{
    MSG_TPY type;
    void * msg;
    int  size;
}MSG;

class forward_server
{
public:
    forward_server(int socket_int,int g_id);
    ~forward_server();
    std::string server_ip;
    bool exit_;
private:
    static void client_rcv(forward_server *this_class);
    static void client_forward(forward_server *this_class);
    static void server_rcv(forward_server *this_class);
    static void server_forward(forward_server *this_class);
    bool server_connect();
    static int send_all(int socket, char *buf,int size);
    std::thread *thr_client_rcv;
    std::thread *thr_client_forward;
    std::thread *thr_server_forward;
    std::thread *thr_server_rcv;
    int server_connect_state;
    bool end_;
    int client_socket;
    int server_socket;
    struct sockaddr_in servaddr;
    int server_port;
private:
    std::queue<MSG> q_server_msg;
    std::queue<MSG> q_client_msg;
    sem_t sem_client_rcv;
    sem_t sem_server_rcv;
public:
    int id;
private:
};



forward_server::forward_server(int socket_int,int g_id) {

    client_socket=socket_int;
    server_socket = socket(AF_INET,SOCK_STREAM, 0);
    end_=false;
    exit_=false;
    id=g_id;
    thr_client_rcv=0;
    thr_client_forward=0;
    thr_server_forward=0;
    thr_server_rcv=0;
    server_connect_state=-2;
    server_port=6019;
    server_ip="172.17.0.240";
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(server_port);  ///服务器端口
    servaddr.sin_addr.s_addr = inet_addr(server_ip.c_str());  ///服务器ip
    thr_client_rcv=new std::thread(forward_server::client_rcv,this);

}

forward_server::~forward_server() {
    printf("id=%d forward_server release start !\n",id);
    end_=true;
    exit_=true;

    sem_post(&sem_client_rcv);
    sem_post(&sem_server_rcv);
    if(thr_client_rcv!=0) {
        thr_client_rcv->join();
        delete  thr_client_rcv;
        thr_client_rcv=0;
    }
    if(thr_client_forward!=0) {
        thr_client_forward->join();
        delete thr_client_forward;
        thr_client_forward = 0;
    }
   if(thr_server_forward!=0) {
       thr_server_forward->join();
       delete thr_server_forward;
       thr_server_forward=0;
   }
   if(thr_server_rcv!=0) {
       thr_server_rcv->join();
       delete thr_server_rcv;
       thr_server_rcv=0;
   }
    printf("id =%d forward_server release join\n",id);


    sem_close(&sem_client_rcv);
    sem_close(&sem_server_rcv);
    while(q_client_msg.size()>0)
    {
        MSG Msg=q_client_msg.front();
        char *buf=(char *)Msg.msg;
        q_client_msg.pop();
        delete [] buf;
    }
    while(q_server_msg.size()>0)
    {
        MSG Msg=q_server_msg.front();
        char *buf=(char *)Msg.msg;
        q_server_msg.pop();
        delete [] buf;
    }
    close(server_socket);
    close(client_socket);
    printf("id=%d forward_server release end !\n",id);
}

void forward_server::client_rcv(forward_server *this_class) {


    printf("id=%d client_rcv star\n",this_class->id);
    if(this_class->server_connect()==false)
    {
        this_class->end_=true;
        this_class->exit_=true;
        return;
    }
    this_class->thr_client_forward=new std::thread(forward_server::client_forward,this_class);
    this_class->thr_server_rcv=new std::thread(forward_server::server_rcv,this_class);
    this_class->thr_server_forward=new std::thread(forward_server::server_forward,this_class);
    while (!this_class->end_)
    {
        char *buffer=new char[BUFFER_SIZE];
        int len = recv(this_class->client_socket, buffer, BUFFER_SIZE, 0);
        if(len>0) {
            printf("client recv len%d\n", len);
            MSG Msg;
            Msg.type=MSG_TPY::msg_client_rcv;
            Msg.msg=buffer;
            Msg.size=len;
            this_class->q_client_msg.push(Msg);
            sem_post(&this_class->sem_client_rcv);
        }
#if 1
        else if(len==0)
        {
            delete [] buffer;
           // printf("id=%d,client recv time out\n",this_class->id);
        }
#endif
        else
        {
            delete [] buffer;
            printf("id =%d client recv erro \n",this_class->id);
        }
        if(len<=0)
        {
            struct tcp_info info;

            int info_len=sizeof(info);

            getsockopt(this_class->client_socket, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&info_len);
            if(info.tcpi_state!=TCP_ESTABLISHED)
            {
                printf("id =%d tcpi_state!=TCP_ESTABLISHED) \n",this_class->id);
                break;
            }
            usleep(1000);
        }
    }

    this_class->end_=true;
    this_class->exit_=true;
    //delete this_class;
    printf("id=%d client_rcv exit!\n",this_class->id);
}

void  forward_server::server_forward(forward_server *this_class) {
   signal(SIGPIPE, SIG_IGN);
    printf("id =%d server_forward start \n",this_class->id);
    while (!this_class->end_) {

        while(this_class->q_client_msg.size()>0)
        {
            MSG Msg=this_class->q_client_msg.front();
            char *buf=(char *)Msg.msg;
            int ret = send_all(this_class->server_socket, buf, Msg.size);
            if (ret < 0) {
                if(this_class->end_)
                {
                    break ;
                }
                printf("id =%d send <0,server_forward\n",this_class->id);
            } else {
                printf("server_forwar =%d\n",Msg.size);
                this_class->q_client_msg.pop();
                delete[] buf;
            }
        }
        sem_wait(&this_class->sem_client_rcv);
    }
    printf("id =%d server_forward exit \n",this_class->id);

}

void forward_server::server_rcv(forward_server *this_class) {

    printf("id=%d server_rcv star! \n",this_class->id);
    while (!this_class->end_)
    {
        char *buffer = new char[BUFFER_SIZE];
        int len = recv(this_class->server_socket, buffer,BUFFER_SIZE, 0);
        if (len > 0) {
           // printf("server recv len%d\n", len);
            MSG Msg;
            Msg.type = MSG_TPY::msg_server_rcv;
            Msg.msg = buffer;
            Msg.size = len;
            this_class->q_server_msg.push(Msg);
            sem_post(&this_class->sem_server_rcv);
        }
#if 1
        else if(len==0)
        {
            delete[] buffer;
            usleep(1000);
           // printf("id=%d,server_rcv,time out\n",this_class->id);
        }
#endif
        else  {
            delete[] buffer;
            usleep(1000);
            printf("id =%d server ,rcv len <0\n",this_class->id);
        }
        if(len<=0)
        {
            struct tcp_info info;

            int info_len=sizeof(info);

            getsockopt(this_class->client_socket, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&info_len);
            if(info.tcpi_state!=TCP_ESTABLISHED)
            {
                printf("id =%d tcpi_state!=TCP_ESTABLISHED) \n",this_class->id);
                break;
            }
            usleep(1000);
        }
    }
    this_class->end_=true;
    this_class->exit_=true;
    printf("id =%d server_rcv exit \n",this_class->id);

}

void  forward_server::client_forward(forward_server *this_class) {
    signal(SIGPIPE, SIG_IGN);
    printf("id=%d client_forward start! \n",this_class->id);
    while (!this_class->end_) {
        while(this_class->q_server_msg.size()>0)
        {
            if(this_class->end_)
            {
                break;
            }
            MSG Msg=this_class->q_server_msg.front();
            char *buf=(char *)Msg.msg;
            int ret = send_all(this_class->client_socket, buf, Msg.size);
            if (ret < 0) {
                printf("id =%d client forward send_all <0,close ,server_socket,client_socket\n",this_class->id);
                usleep(1000);
            } else
            {
                //printf("client_forward len=%d\n",Msg.size);
            }
            this_class->q_server_msg.pop();
            delete [] buf;
        }
        sem_wait(&this_class->sem_server_rcv);
    }
    printf("id=%d client_forward exit! \n",this_class->id);
}

int forward_server::send_all(int socket, char *buf,int size)
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

bool forward_server::server_connect()
{
    printf("id=%d server_connect star! \n",id);
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
            printf("id=%d server tv.tv_sec=%ld,tv_usec=%ld \n",id,tv.tv_sec,tv.tv_usec);

#endif
            printf("id =%d ,server connect suc \n",id);
            server_connect_state = 1;
    }
    printf("id =%d server_connect exit \n",id);
    return true;
}
#endif //PROJECT_FORWAR_SERVER_H