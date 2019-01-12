//
// Created by czx on 19-1-6.
//

#ifndef GFW_RINGBUF_H
#define GFW_RINGBUF_H
class RingBuffer
{
public:
    RingBuffer();
    ~RingBuffer();
    unsigned char * alloc(unsigned int &size);
    void w_back(unsigned int size);
    void free(unsigned int size);
private:
    unsigned int total_size;
    unsigned char *Data_begin;
    unsigned char *Data_end;
    unsigned char *r_cursor;
    unsigned char  *w_cursor;
    unsigned int  free_size;
};
RingBuffer::RingBuffer()
{
    total_size=1024*1024*32;
    Data_begin=new unsigned char [total_size];
    Data_end=Data_begin+total_size-1;
    r_cursor=Data_begin;
    free_size=total_size;
}
RingBuffer::~RingBuffer()
{
    delete[] Data_begin;
}
unsigned char * RingBuffer::alloc(unsigned int &size)
{
    unsigned char *ret_buf=w_cursor;
    int  r_remain=Data_end-w_cursor+1;
    size=size<free_size?size:free_size;
    size=size<r_remain?size:r_remain;
    w_cursor+=size;
    free_size-=size;
    if(w_cursor>Data_end)
    {
        w_cursor=Data_begin;
    }
    return ret_buf;
}

void RingBuffer::w_back(unsigned int size)
{
    w_cursor-=size;
    if(w_cursor<Data_begin)
    {
        w_cursor=Data_end;
    }
    free_size+=size;
}

void RingBuffer::free(unsigned int size)
{
    free_size-=size;
}

#endif //GFW_RINGBUF_H