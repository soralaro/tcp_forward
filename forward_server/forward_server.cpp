//
// Created by czx on 18-12-16.
//
#include "forward_server.h"
#define PORT 7002
#define QUEUE 20
#define STOP_SSR  0x11
#define STAR_SSR  0x22

void commandPro(unsigned char command)
{
    switch(command)
    {
        case STAR_SSR:
            system("/home/ubuntu/ssr_star_com.sh");
            break;
        case STOP_SSR:
            system("/home/ubuntu/ssr_stop_com.sh");
            break;
        default:
            break;
    }
}
int end_=0;
static int forward_release_num=0;
static int forward_connect_num=0;
std::vector<forward_server *> v_forward_server;
void freed_forword()
{
    while(!end_)
    {
        usleep(1000);
        for(std::vector<forward_server *>::iterator it=v_forward_server.begin();it!=v_forward_server.end();)
        {
           if((*it)->exit_)
           {
               printf("freed_forward =%p id=%d\n",(*it),(*it)->id);
               delete (*it);
               v_forward_server.erase(it);
               forward_release_num++;
               break;
           }
        }

    }

}
int main() {
    signal(SIGPIPE, SIG_IGN);
    int conn;
    //std::thread freed_forward_thr(freed_forword);
    int ss = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_sockaddr;
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(PORT);
    //printf("%d\n",INADDR_ANY);
    server_sockaddr.sin_addr.s_addr = inet_addr("172.17.0.240");  ///服务器ip //htonl(INADDR_ANY);
    if(bind(ss, (struct sockaddr* ) &server_sockaddr, sizeof(server_sockaddr))==-1) {
        perror("bind");
        exit(1);
    }
    if(listen(ss, QUEUE) == -1) {
        perror("listen");
        exit(1);
    }

    struct sockaddr_in client_addr;
    socklen_t length = sizeof(client_addr);
    while(1) {
        ///成功返回非负描述字，出错返回-1
        printf("waiting for connet!\n");
        conn = accept(ss, (struct sockaddr *) &client_addr, &length);
        if (conn < 0) {
            perror("connect");
            //exit(1);
            continue;
        }

        static int g_id=0;
#if 1
        struct timeval timeout = {3, 0};//3s
        int ret = setsockopt(conn, SOL_SOCKET, SO_SNDTIMEO,  &timeout, sizeof(timeout));
        if (ret < 0) {
            perror("setsockopt SO_SNDTIMEO");
            exit(1);
        }
        ret = setsockopt(conn, SOL_SOCKET, SO_RCVTIMEO,  &timeout, sizeof(timeout));
        if (ret < 0) {
            perror("setsockopt SO_RCVTIMEO");
            exit(1);
        }

        socklen_t optlen = sizeof(struct timeval);

        struct timeval tv;

        memset(&tv,0,sizeof(tv));

        getsockopt(conn, SOL_SOCKET,SO_RCVTIMEO, &tv, &optlen);
        printf("id=%d clien tv.tv_sec=%ld,tv_usec=%ld \n",g_id,tv.tv_sec,tv.tv_usec);
#endif
        printf("client connect suc,forward_connect_num=%d,forward_release_num=%d\n",forward_connect_num++,forward_release_num);
        forward_server *forward = new forward_server(conn,g_id++);

        printf("%s\n", forward->server_ip.c_str());
        v_forward_server.push_back(forward);
    }
    close(ss);

    return 0;
}
