#include <iostream>
#include <string>
#include <string.h>
#include "encrypt.h"
#include "gdb_log.h"
#define LENGTH 16
#define EN0	0	/* MODE == encrypt */
#define DE1	1	/* MODE == decrypt */
extern "C" {
extern void des2key(unsigned char *, short);
extern void des2key_2(unsigned char *, short);
extern void des2key_3(unsigned char *, short);
extern void D2des(unsigned char *, unsigned char *);
extern void D2des_2(unsigned char *, unsigned char *);
extern void D2des_3(unsigned char *, unsigned char *);
extern void des2key_de(unsigned char *, short);
extern void des2key_de_2(unsigned char *, short);
extern void des2key_de_3(unsigned char *, short);
extern void D2des_de(unsigned char *, unsigned char *);
extern void D2des_de_2(unsigned char *, unsigned char *);
extern void D2des_de_3(unsigned char *, unsigned char *);
}
using namespace std;
static const char *default_key = "~!@#$%^&*()_+QWE";

void des_encrypt_init(const char *key)
{
    if(key==NULL) {
        des2key((unsigned char *) default_key, EN0);
        des2key_de((unsigned char *) default_key, DE1);
    }
    else
    {
        des2key((unsigned char *) key, EN0);
        des2key_de((unsigned char *) key, DE1);
    }
}
void des_encrypt_init_2(const char *key)
{
    if(key==NULL) {
        des2key_2((unsigned char *) default_key, EN0);
        des2key_de_2((unsigned char *) default_key, DE1);
    }
    else
    {
        des2key_2((unsigned char *) key, EN0);
        des2key_de_2((unsigned char *) key, DE1);
    }
}

void des_encrypt_init_3(const char *key)
{
    if(key==NULL) {
        des2key_3((unsigned char *) default_key, EN0);
        des2key_de_3((unsigned char *) default_key, DE1);
    }
    else
    {
        des2key_3((unsigned char *) key, EN0);
        des2key_de_3((unsigned char *) key, DE1);
    }
}

void des_encrypt(unsigned char *buf,int size)
{
    unsigned char encrypt[LENGTH];
    unsigned char *p=buf;
    for(int i=0;i<size;i+=LENGTH)
    {
        D2des(p,encrypt);
        if(size-i<LENGTH)
            memcpy(p,encrypt,size-i);
        else
            memcpy(p,encrypt,LENGTH);
        p+=LENGTH;
    }
}
void des_decrypt(unsigned char *buf,int size)
{
    unsigned char encrypt[LENGTH];
    unsigned char *p=buf;
    for(int i=0;i<size;i+=LENGTH)
    {
        D2des_de(p,encrypt);
        if(size-i<LENGTH)
            memcpy(p,encrypt,size-i);
        else
            memcpy(p,encrypt,LENGTH);
        p+=LENGTH;
    }
}
void des_encrypt_2(unsigned char *buf,int size)
{
    unsigned char encrypt[LENGTH];
    unsigned char *p=buf;
    for(int i=0;i<size;i+=LENGTH)
    {
        D2des_2(p,encrypt);
        if(size-i<LENGTH)
            memcpy(p,encrypt,size-i);
        else
            memcpy(p,encrypt,LENGTH);
        p+=LENGTH;
    }
}
void des_decrypt_2(unsigned char *buf,int size)
{
    unsigned char encrypt[LENGTH];
    unsigned char *p=buf;
    for(int i=0;i<size;i+=LENGTH)
    {
        D2des_de_2(p,encrypt);
        if(size-i<LENGTH)
            memcpy(p,encrypt,size-i);
        else
            memcpy(p,encrypt,LENGTH);
        p+=LENGTH;
    }
}

void des_encrypt_3(unsigned char *buf,int size)
{
    if(size==0)
    {
        return;
    }
    unsigned char encrypt[LENGTH];
    unsigned char buf_last_first[LENGTH];
    unsigned char *p=buf;
    if(size%LENGTH!=0)
    {
        DGFAT("size%LENGTH!=0");
    }
    int half_len=LENGTH/2;
    int num=size/LENGTH;
    p+=half_len;
    for(int i=1;i<num;i++)
    {
        D2des_3(p,encrypt);
        memcpy(p,encrypt,LENGTH);
        p+=LENGTH;
    }
    memcpy(buf_last_first,buf+size-half_len,half_len);
    memcpy(buf_last_first+half_len,buf,half_len);
    D2des_3(buf_last_first,encrypt);
    memcpy(buf+size-half_len,encrypt,half_len);
    memcpy(buf,encrypt+half_len,half_len);
}
void des_decrypt_3(unsigned char *buf,int size)
{
    if(size==0)
    {
        return;
    }
    unsigned char encrypt[LENGTH];
    unsigned char buf_last_first[LENGTH];
    unsigned char *p=buf;
    if(size%LENGTH!=0)
    {
        DGFAT("size%LENGTH!=0");
    }
    int half_len=LENGTH/2;
    int num=size/LENGTH;
    p+=half_len;
    for(int i=1;i<num;i++)
    {
        D2des_de_3(p,encrypt);
        memcpy(p,encrypt,LENGTH);
        p+=LENGTH;
    }
    memcpy(buf_last_first,buf+size-half_len,half_len);
    memcpy(buf_last_first+half_len,buf,half_len);
    D2des_de_3(buf_last_first,encrypt);
    memcpy(buf+size-half_len,encrypt,half_len);
    memcpy(buf,encrypt+half_len,half_len);
}

#if 0

int main(int argc, char *argv[])
{
    long lSize;
    char * buffer;
    size_t result;

	FILE * pFile = fopen ( argv[1] , "rb" );
	if (pFile==NULL)
    {
        fputs ("File error",stderr);
        exit (1);
    }

	fseek (pFile , 0 , SEEK_END);
	lSize = ftell (pFile);
	rewind (pFile);

	buffer = (char*) malloc (sizeof(char)*lSize);
	if (buffer == NULL)
    {
        fputs ("Memory error",stderr);
        exit (2);
    }

	result = fread (buffer,1,lSize,pFile);
	if (result != lSize)
    {
        fputs ("Reading error",stderr);
        exit (3);
    }

	printf("file length = %ld\n", lSize);
	//printf("orig file content = %s\n", buffer);

    string raw_data(buffer, lSize);
    string encrypt_data;
    do_encryption(raw_data, encrypt_data);

    const char *post = ".model";

    int fnlen = strlen(argv[1]) + strlen(post) + 1;
    char *target = malloc(fnlen);
    target[0] = 0;
    strcat(target, argv[1]).strcat(target, post);

    FILE *fp = fopen(target, "wb");
    if (fp == NULL)
    {
        fprintf(stderr, "file not found\n");;
        return 0;
    }
    result = fwrite (encrypt_data.c_str() , sizeof(char), encrypt_data.length(), fp);
    fclose (fp);


    printf("encrypt write file length %ld\n", result);

    FILE * pF_en = fopen ( "encrypted_file.model" , "rb" );
    if (pF_en==NULL)
    {
        fputs ("File error",stderr);
        exit (1);
    }

    fseek (pF_en , 0 , SEEK_END);
    lSize = ftell (pF_en);
    rewind (pF_en);

    char *buffer_en = (char*) malloc (sizeof(char)*lSize);
    if (buffer_en == NULL)
    {
        fputs ("Memory error",stderr);
        exit (2);
    }

    result = fread (buffer_en, 1, lSize, pF_en);
    if (result != lSize)
    {
        fputs ("Reading error",stderr);
        exit (3);
    }

    string raw_data_en(buffer_en, lSize);
    string decrypt_data;

    do_decryption(raw_data_en, decrypt_data);

    const char *output_file_name_de = "decrypted_file.model";

    FILE *fp_de = fopen(output_file_name_de, "wb");
    if (fp_de == NULL)
    {
        fprintf(stderr, "file not found\n");;
        return 0;
    }
    result = fwrite (decrypt_data.c_str() , sizeof(char), decrypt_data.length(), fp_de);

    printf("decrypt write file length %ld\n", result);

    fclose (pFile);
    free (buffer);

    return 0;
}
#endif
#if 0
int main(int argc, char *argv[])
{
    long lSize;
    char * buffer;
    size_t result;

    if(argc != 3) {
        fprintf(stderr, "usage %s: <src> <dst>", argv[0]);
        exit(4);
    }

	FILE * pFile = fopen ( argv[1] , "rb" );
	if (pFile==NULL)
    {
        fputs ("File error",stderr);
        exit (1);
    }

	fseek (pFile , 0 , SEEK_END);
	lSize = ftell (pFile);
	rewind (pFile);

	if(lSize % 16 != 0)
    {
	    fputs("file size not multiple 16", stderr);
	    exit (5);
    }

	buffer = (char*) malloc (sizeof(char)*lSize);
	if (buffer == NULL)
    {
        fputs ("Memory error",stderr);
        exit (2);
    }

	result = fread (buffer,1,lSize,pFile);
	if (result != lSize)
    {
        fputs ("Reading error",stderr);
        exit (3);
    }
    fclose (pFile);

	printf("file length = %ld\n", lSize);
	//printf("orig file content = %s\n", buffer);

    string raw_data(buffer, lSize);
    string encrypt_data;
    do_encryption(raw_data, encrypt_data);

    const char *target = argv[2];
    FILE *fp = fopen(target, "wb");
    if (fp == NULL)
    {
        fprintf(stderr, "destination file %s not found\n", target);;
        exit (6);
    }
    result = fwrite (encrypt_data.c_str() , sizeof(char), encrypt_data.length(), fp);
    fclose (fp);


    printf("encrypt write file length %ld\n", result);

    free (buffer);

    return 0;
}
#endif