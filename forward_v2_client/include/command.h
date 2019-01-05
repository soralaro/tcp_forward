//
// Created by czx on 19-1-5.
//

#ifndef GFW_COMMAND_H
#define GFW_COMMAND_H
enum class socket_command
        {
              Data =1,
              connect =2,
              dst_connetc=3
        };
typedef struct COM_struct
{
    unsigned int size;
    unsigned int  com;
    int   cl_socket;
}command;
#endif //GFW_COMMAND_H
