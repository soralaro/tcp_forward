//
// Created by czx on 18-12-16.
//

#include"omp.h"
#include "../include/server.h"
#include "../include/forward.h"
#include <algorithm>
#include "encrypt.h"


unsigned char server::encryp_key=0x55;
unsigned char server::encryp_key_2=0xae;
char server::des_key[17];
char server::des_key_2[17];
char server::des_key_3[17];
std::vector<server *> server::server_Pool;
std::map<int ,int> server::mapUsr;
std::mutex server::mapUsr_lock;
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
    for(int i=sizeof(COMMANT)*2;i<len;i++)
    {
        if(i!=(encryp_key_2+sizeof(COMMANT)))
            buf[i]=(buf[i]^buf[encryp_key_2+sizeof(COMMANT)]^encryp_key_2);
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
            server_Pool[i]->free= false;
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
void server::setDesKey(char *key)
{
    memset(des_key,0,sizeof(des_key));
    memcpy(des_key,key,sizeof(des_key)-1);
    des_encrypt_init(des_key);
};
void server::setDesKey_2(char *key)
{
    memset(des_key_2,0,sizeof(des_key_2));
    memcpy(des_key_2,key,sizeof(des_key_2)-1);
    des_encrypt_init_2(des_key_2);
};
void server::setDesKey_3(char *key)
{
    memset(des_key_3,0,sizeof(des_key_3));
    memcpy(des_key_3,key,sizeof(des_key_3)-1);
    des_encrypt_init_3(des_key_3);
}
server::server()
{
    end_=true;
    free=true;
    id=0;
    usr_id=-1;
    destroy=false;
    client_socket=0;
    forward_end=true;
    rcv_end=true;
    connect_exist_time=0;
    waite_forward_end_state=false;
    ThreadPool::pool_add_worker(server_rcv, this);
    ThreadPool::pool_add_worker(forward, this);
    ThreadPool::pool_add_worker(timer_fuc, this);
    commandProcess=new command_process(&q_client_msg);
    commandProcess->usr_id=&usr_id;
    commandProcess->mapUsr=&mapUsr;
    commandProcess->mapUsr_lock=&mapUsr_lock;
    encry_data=NULL;
}
server::~server()
{
    delete commandProcess;
    if(encry_data!=NULL)
    {
        delete [] encry_data;
        encry_data=NULL;
    }
}
void server::init(unsigned int g_id,int socket_int,struct sockaddr_in addr) {

    connect_exist_time=0;
    encry_data=new unsigned char [BUFFER_SIZE];
    srand((int)time(0));
    for(int i=0;i<BUFFER_SIZE;)
    {
        encry_data[i]=rand();
        if(encry_data[i]!=0)
        {
           // printf("%x ",encry_data[i]);
            i++;
        }
    }
    printf("\n");
    while (!q_client_msg.empty()) {
        MSG Msg;
        q_client_msg.pop(Msg);
        char *buf = (char *) Msg.msg;
        delete[] buf;
    }
    usr_id=-1;
    commandProcess->encry_data=encry_data;

    MSG Msg;
    Msg.type = MSG_TPY::msg_encrypt;
    Msg.socket_id=g_id;
    char *buffer = new char[BUFFER_SIZE+sizeof(COMMANT)];
    Msg.msg = buffer;
    Msg.size=BUFFER_SIZE+sizeof(COMMANT);
    COMMANT commant;
    commant.size=BUFFER_SIZE+sizeof(COMMANT);
    commant.com=(unsigned int)socket_command::encrypt;
    commant.socket_id=1;
    memcpy(buffer,&commant,sizeof(commant));
    memcpy(buffer+sizeof(COMMANT),encry_data,BUFFER_SIZE);
    q_client_msg.push(Msg);


    client_socket=socket_int;
    client_addr=addr;
    commandProcess->server_end=&end_;
    commandProcess->client_addr=&client_addr;
    id=g_id;
    end_=false;
    send_sn=0;
    heart_beat=0;
    std::unique_lock<std::mutex> mlock(mutex_client_socket);
    mlock.unlock();
    cond_client_socket.notify_all();
    std::unique_lock<std::mutex> mlock2(mutex_forward_start);
    mlock2.unlock();
    cond_forward_start.notify_all();

}
void server::release()
{
    waite_forward_end_lock.lock();
    if(!forward_end)
    {
        MSG msg;
        msg.type=MSG_TPY::msg_server_release;
        msg.size=0;
        msg.msg=NULL;
        q_client_msg.push(msg);
        std::unique_lock<std::mutex> mlock(mutex_forward_end);
        waite_forward_end_state=true;
        waite_forward_end_lock.unlock();
        cond_forward_end.wait(mlock);
        waite_forward_end_state=false;
    } else{
        waite_forward_end_lock.unlock();
    }

    commandProcess->relase();
    while (!q_client_msg.empty()) {
        MSG Msg;
        q_client_msg.pop(Msg);
        char *buf = (char *) Msg.msg;
        delete[] buf;
    }
    if(encry_data!=NULL)
    {
        delete [] encry_data;
        encry_data=NULL;
    }
    DGDBG("id=%d release end !\n",id);
    int dev_num=0;
    if(usr_id!=-1)
    {
        mapUsr_lock.lock();
        auto iter = mapUsr.find(usr_id);
        if(iter!=mapUsr.end())
        {
            iter->second--;
            dev_num=iter->second;
            if(iter->second<=0)
            {
                mapUsr.erase(iter);
            }
        }
        mapUsr_lock.unlock();
    }
    commandProcess->log(0,usr_id,dev_num);
    id=0;
    usr_id=-1;
    close(client_socket);
    free=true;
    heart_beat=0;
    connect_exist_time=0;
}

void server::timer_fuc(void *arg)
{
    server *this_class = (server *)arg;
    while(!this_class->destroy) {
        sleep(3);
        if(!this_class->end_) {
            this_class->heart_beat++;
            this_class->connect_exist_time++;
            if(this_class->connect_exist_time>3600*24/3)
            {
                MSG msg;
                msg.type=MSG_TPY::msg_server_release;
                msg.size=0;
                msg.msg=NULL;
                this_class->q_client_msg.push(msg);
                this_class->connect_exist_time=0;
            }
            if (this_class->heart_beat > 10)
            {
                this_class->end_= true;
                this_class->heart_beat=0;
            }
            else
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

void server::server_rcv(void *arg) {

    server *this_class = (server *)arg;

    std::unique_lock<std::mutex> mlock(this_class->mutex_client_socket);
    while (!this_class->destroy) {
        this_class->cond_client_socket.wait(mlock);
        this_class->rcv_end=false;
        DGDBG("id=%d server_rcv star! \n",this_class->id);
        while (!this_class->end_) {
            char buffer[BUFFER_SIZE];

            int len = recv(this_class->client_socket, buffer, BUFFER_SIZE, 0);
            if (len > 0) {
                 DGDBG("server recv len%d\n", len);
                this_class->commandProcess->process((unsigned char *)buffer, len);
                this_class->heart_beat=0;
            } else {
                struct tcp_info info;

                int info_len = sizeof(info);

                getsockopt(this_class->client_socket, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *) &info_len);
                if (info.tcpi_state != TCP_ESTABLISHED) {
                    DGDBG("id =%d tcpi_state!=TCP_ESTABLISHED) \n",this_class->id);
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
    unsigned  char ex_buf[128];
    std::unique_lock<std::mutex> mlock(this_class->mutex_forward_start);
    while (!this_class->destroy) {
        this_class->cond_forward_start.wait(mlock);
        this_class->forward_end = false;
        signal(SIGPIPE, SIG_IGN);
        DGDBG("id=%d server_forward star! \n",this_class->id);
        while (!this_class->end_) {
            MSG Msg;
            this_class->q_client_msg.pop(Msg);
            if(Msg.type==MSG_TPY::msg_server_release)
            {
                if(Msg.size>0)
                {
                    char *buf=(char *)Msg.msg;
                    delete[] buf;
                }
                break;
            }
            if(Msg.type==MSG_TPY::msg_socket_end)
            {
                this_class->commandProcess->erease_mforward(Msg.socket_id);
            }

            if(Msg.type==MSG_TPY::msg_client_rcv)
            {
                if(!this_class->commandProcess->check_mforward_exist(Msg.socket_id))
                {
                    if(Msg.size>0)
                    {
                        char *buf=(char *)Msg.msg;
                        delete[] buf;
                    }
                    DGDBG("socket_id=%d,has not existed! ",Msg.socket_id);
                    continue;
                }
            }

            if(Msg.size>0) {
                char *buf = (char *) Msg.msg;
                COMMANT commant;
                memcpy(&commant, buf, sizeof(commant));
                commant.sn = this_class->send_sn++;
                commant.res0 =rand();
                commant.res1=rand();
                commant.res2=rand();
                commant.res3=rand();
                commant.res4=rand();
                commant.ex_size=rand();
                DGDBG("server_forward_commant size=%x,sn=%x,id=%x,com=%x ", commant.size, commant.sn, commant.socket_id,
                      commant.com);
                memcpy(buf, &commant, sizeof(commant));
                int align_len=ALIGN_16(Msg.size)-Msg.size;
                unsigned char *p=(unsigned char *)(buf+Msg.size);
                for(int i=0;i<align_len;i++)
                {
                    p[i]=rand();
                }
                if (Msg.type == MSG_TPY::msg_encrypt) {
                    this_class->data_cover((unsigned char *) buf, Msg.size);

                }
                else {
                    this_class->data_encrypt((unsigned char *) buf, Msg.size);
                }

                des_encrypt(buf,sizeof(commant));
                des_encrypt_3(buf,sizeof(commant));

                des_encrypt_2(buf+sizeof(commant),ALIGN_16(Msg.size-sizeof(commant)));

                des_encrypt_3(buf+sizeof(commant),ALIGN_16(Msg.size-sizeof(commant)));

                int ret = this_class->send_all(buf, ALIGN_16(Msg.size));

                delete[] buf;
                DGERR("command size=%d,ex_size=%d",commant.size,commant.ex_size);
                if(ret>0)
                {
                    int ex_len=0x7f&commant.ex_size;
                    for(int i=0;i<ex_len;i++)
                    {
                        ex_buf[i]=rand();
                    }

                    ret = this_class->send_all((char *)ex_buf, ex_len);
                }
                if (ret < 0) {
                    // DGDBG("id =%d send <0,server_forward\n",this_class->id);
                    break;
                } else {
                    DGDBG("server_forwar send size =%d\n", Msg.size);
                }
            }
            if(Msg.type==MSG_TPY::msg_client_expire||Msg.type==MSG_TPY::msg_exit_client)
            {
                break;
            }

        }
        this_class->waite_forward_end_lock.lock();
        this_class->end_=true;
        this_class->forward_end=true;
        if(this_class->waite_forward_end_state) {
            std::unique_lock<std::mutex> mlock(this_class->mutex_forward_end);
            mlock.unlock();
            this_class->cond_forward_end.notify_all();
        }
        this_class->waite_forward_end_lock.unlock();
    }

    DGDBG("id =%d server_forward exit \n",this_class->id);

}

int server::send_all( char *buf,int size)
{
    int ret;
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
            end_=true;
            return -1;
        }
    }
    return 1;
}



