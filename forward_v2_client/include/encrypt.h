//
// Created by deepglint on 19-2-4.
//

#ifndef GFW_ENCRYPT_H
#define GFW_ENCRYPT_H
#include <string>
#include <string.h>
#define ALIGN_16(x) (((x)+15)/16*16)
void des_encrypt_init(const char *key);

void des_encrypt(unsigned char *buf,int size);
void des_decrypt(unsigned char *buf,int size);
void des_encrypt_init_2(const char *key);

void des_encrypt_2(unsigned char *buf,int size);
void des_decrypt_2(unsigned char *buf,int size);
void des_encrypt_init_3(const char *key);

void des_encrypt_3(unsigned char *buf,int size);
void des_decrypt_3(unsigned char *buf,int size);
#endif //GFW_ENCRYPT_H
