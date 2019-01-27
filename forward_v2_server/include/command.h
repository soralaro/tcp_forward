//
// Created by czx on 19-1-5.
//

#ifndef GFW_COMMAND_H
#define GFW_COMMAND_H
#define BUFFER_SIZE 1024

enum class socket_command
        {
              Data =0x1234,
              connect =0x1235,
              dst_connetc=0x1236,
              encrypt=0x1237,
              heart_beat=0x1238
        };
typedef struct COM_struct
{
    unsigned int size;
    unsigned int sn;
    unsigned int  com;
    unsigned int socket_id;
}COMMANT;
enum class MSG_TPY
{
    msg_client_rcv=0,
    msg_server_rcv,
    msg_socket_end,
    msg_encrypt,
    msg_server_release,
    msg_heart_beat
};

typedef struct MSG_struct
{
    MSG_TPY type;
    unsigned int socket_id;
    void * msg;
    int  size;
}MSG;
#endif //GFW_COMMAND_H
