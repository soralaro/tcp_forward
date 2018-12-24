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

#define BUFFER_SIZE 1024*1024

 enum class MSG_TPY
{
    msg_connect_server=0,
    msg_dis_connect_server,
    msg_client_rcv,
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
    static void server_connect(forward_server *this_class);
    static int send_all(int socket, char *buf,int size);
    std::thread *thr_client_rcv;
    std::thread *thr_client_forward;
    std::thread *thr_server_forward;
    std::thread *thr_server_rcv;
    std::thread *thr_server_connect;
    int server_connect_state;
    bool end_;
    int client_socket;
    int server_socket;
    struct sockaddr_in servaddr;
    int server_port;
private:
    std::queue<MSG> q_server_msg;
    std::queue<MSG> q_client_msg;
   // sem_t sem_sever_connet;
   // sem_t sem_sever_conneted;
    //sem_t sem_client_rcv;
   // sem_t sem_server_rcv;
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
    server_connect_state=-2;
    server_port=6019;
    server_ip="172.17.0.240";
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(server_port);  ///服务器端口
    servaddr.sin_addr.s_addr = inet_addr(server_ip.c_str());  ///服务器ip
    thr_client_rcv=new std::thread(forward_server::client_rcv,this);
    thr_client_forward=new std::thread(forward_server::client_forward,this);
    thr_server_forward=new std::thread(forward_server::server_forward, this);
    thr_server_rcv=new std::thread(forward_server::server_rcv, this);
    thr_server_connect=new std::thread(forward_server::server_connect, this);
}

forward_server::~forward_server() {
    printf("id=%d forward_server release start !\n",id);
    end_=true;
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
    thr_client_rcv->join();
    thr_client_forward->join();
    thr_server_connect->join();
    thr_server_forward->join();
    thr_server_rcv->join();
    printf("id =%d forward_server release join\n",id);
    delete  thr_client_rcv;
    delete thr_client_forward;
    delete thr_server_connect;
    delete thr_server_forward;
    delete thr_server_rcv;

    //sem_close(&sem_sever_connet);
    //sem_close(&sem_sever_conneted);
    //sem_close(&sem_client_rcv);
    //sem_close(&sem_server_rcv);


    printf("id=%d forward_server release end !\n",id);
}

void forward_server::client_rcv(forward_server *this_class) {


    printf("id=%d client_rcv star\n",this_class->id);

    //sem_post(&sem_sever_connet);

    this_class->server_connect_state=-1;
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
            //sem_post(&sem_client_rcv);
        }
#if 1
        else if(len==0)
        {
            delete [] buffer;
            usleep(1000);
            //printf("client recv time out\n");
            //break;
        }
#endif
        else
        {
            delete [] buffer;
            printf("id =%d client recv erro \n",this_class->id);
            //break;
        }
    }
    close(this_class->server_socket);

    close(this_class->client_socket);
    this_class->end_=true;
    usleep(1000*10);
    this_class->exit_=true;
    //delete this_class;
    printf("id=%d client_rcv exit!\n",this_class->id);
}

void  forward_server::server_forward(forward_server *this_class) {
    //signal(SIGPIPE, SIG_IGN);
    printf("id =%d server_forward start \n",this_class->id);
    while (!this_class->end_) {
        // sem_wait(&sem_client_rcv);
        //if(end_)
        // {
        //      break;
        // }
        //usleep(1000);
        while(this_class->q_client_msg.size()>0)
        {
            while(this_class->server_connect_state==-1)
            {
                if(this_class->end_)
                {
                    break;
                }
                usleep(1000);
            }
            if(this_class->end_)
            {
                break;
            }
            MSG Msg=this_class->q_client_msg.front();
            char *buf=(char *)Msg.msg;
            int ret = send_all(this_class->server_socket, buf, Msg.size);
            if (ret < 0) {
               // close(this_class->server_socket);
                if(this_class->end_)
                {
                    break ;
                }
                printf("id =%d send <0,server_forward,close s server_socket\n",this_class->id);
                this_class->server_connect_state = -1;
                //sem_post(&sem_sever_connet);;
            } else {
                printf("server_forwar =%d\n",Msg.size);
                this_class->q_client_msg.pop();
                delete[] buf;
            }
        }
    }
    printf("id =%d server_forward exit \n",this_class->id);

}

void forward_server::server_rcv(forward_server *this_class) {

    printf("id=%d server_rcv star! \n",this_class->id);
    while (!this_class->end_)
    {
        //sem_wait(&sem_sever_conneted);
        // if(end_)
        // {
        //     break;
        // }
        usleep(1000);
        while(this_class->server_connect_state==1) {
            if(this_class->end_)
            {
                break;
            }
            char *buffer = new char[BUFFER_SIZE];
            int len = recv(this_class->server_socket, buffer,BUFFER_SIZE, 0);
            if (len > 0) {
               // printf("server recv len%d\n", len);
                MSG Msg;
                Msg.type = MSG_TPY::msg_server_rcv;
                Msg.msg = buffer;
                Msg.size = len;
                this_class->q_server_msg.push(Msg);
                //sem_post(&sem_server_rcv);
            }
#if 1
            else if(len==0)
            {
                delete[] buffer;
                usleep(1000);
                //printf("server_rcv,time out\n");
            }
#endif
            else  {
                delete[] buffer;
               // close(this_class->server_socket);
                if(this_class->end_)
                {
                    break;
                }
                this_class->server_connect_state = -1;
                //sem_post(&sem_sever_connet);
                printf("id =%d server ,rcv len <0,close server_socket\n",this_class->id);
            }
        }

    }
    printf("id =%d server_rcv exit \n",this_class->id);

}

void  forward_server::client_forward(forward_server *this_class) {
   // signal(SIGPIPE, SIG_IGN);
    printf("id=%d client_forward start! \n",this_class->id);
    while (!this_class->end_) {
       // sem_wait(&sem_server_rcv);
      //  if(end_)
      //  {
     //       break;
      //  }
       usleep(1000);
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
                //close(this_class->server_socket);
                //close(this_class->client_socket);
                return;
            } else
            {
                //printf("client_forward len=%d\n",Msg.size);
            }
            this_class->q_server_msg.pop();
            delete [] buf;
        }
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

void forward_server::server_connect(forward_server *this_class)
{
    printf("id=%d server_connect star! \n",this_class->id);
        if (connect(this_class->server_socket, (struct sockaddr *) &this_class->servaddr, sizeof(servaddr)) < 0) {
           // perror("server connect");
            close(this_class->server_socket);
            close(this_class->client_socket);
           return; 
            usleep(100*1000);
        }
        else {
#if 0
            struct timeval timeout = {3, 0};//3s
            int ret = setsockopt(this_class->server_socket, SOL_SOCKET, SO_SNDTIMEO,  &timeout, sizeof(timeout));
            if (ret < 0) {
                perror("setsockopt SO_SNDTIMEO");
                exit(1);
            }
            ret = setsockopt(this_class->server_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
            if (ret < 0) {
                perror("setsockopt SO_RCVTIMEO");
                exit(1);
            }

            socklen_t optlen = sizeof(struct timeval);

            struct timeval tv;

            memset(&tv,0,sizeof(tv));

            getsockopt(this_class->server_socket, SOL_SOCKET,SO_RCVTIMEO, &tv, &optlen);
            printf("id=%d server tv.tv_sec=%ld,tv_usec=%ld \n",this_class->id,tv.tv_sec,tv.tv_usec);

#endif
            printf("id =%d ,server connect suc \n",this_class->id);
            this_class->server_connect_state = 1;
            //sem_post(&sem_sever_conneted);
        }
    printf("id =%d server_connect exit \n",this_class->id);
}
#endif //PROJECT_FORWAR_SERVER_H