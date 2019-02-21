//
// Created by czx on 19-1-5.
//

#ifndef GFW_COMMAND_H
#define GFW_COMMAND_H
#define BUFFER_SIZE 1024

enum class socket_command
{
    Data =0x34,
    connect =0x35,
    dst_connetc=0x36,
    encrypt=0x37,
    heart_beat=0x38,
    user_expire=0x39
};
typedef struct COM_struct
{
    unsigned short size;
    unsigned char com;
    unsigned char res0;
    unsigned int  sn;
    unsigned int  user_id;
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
