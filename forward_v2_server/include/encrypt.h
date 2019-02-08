//
// Created by deepglint on 19-2-4.
//

#ifndef GFW_ENCRYPT_H
#define GFW_ENCRYPT_H
#include <string>
#include <string.h>

void des_encrypt_init(const char *key);
void do_encryption(std::string& file_to_be_encrypt, std::string& encrypted_file);
void do_decryption(std::string& file_to_be_decrpt, std::string& decrpted_file);
void des_encrypt(unsigned char *buf,int size);
void des_decrypt(unsigned char *buf,int size);
#endif //GFW_ENCRYPT_H
