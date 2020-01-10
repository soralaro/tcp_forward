//
// Created by czx on 19-1-5.
//

#ifndef GFW_COMMAND_H
#define GFW_COMMAND_H
#define BUFFER_SIZE 1024
#define EX_SIZE 272
#define ONE_USER_MAX_DEVICE 5

enum class socket_command
{
    Data =0x34,
    connect =0x35,
    dst_connetc=0x36,
    encrypt=0x37,
    heart_beat=0x38,
    user_expire=0x39,
    exceed_max_device=0x3a
};
typedef struct COM_struct {
    unsigned int rand[8];
    unsigned int res0;
    unsigned short size;
    unsigned char com;
    unsigned char res1;
    unsigned int sn;
    unsigned int res2;
    unsigned short ex_size;
    unsigned short user_id;
    unsigned int res3;
    unsigned int res4;
    unsigned int socket_id;
} COMMANT;

enum class MSG_TPY
{
    msg_client_rcv=0,
    msg_server_rcv,
    msg_socket_end,
    msg_encrypt,
    msg_server_release,
    msg_heart_beat,
    msg_client_expire,
    msg_exit_client,
    msg_ext_data,
    msg_res
};

typedef struct MSG_struct
{
    MSG_TPY type;
    unsigned int socket_id;
    void * msg;
    int  size;
}MSG;
#endif //GFW_COMMAND_H
