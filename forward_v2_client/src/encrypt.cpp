#include <iostream>
#include <string>
#include <string.h>
#include "d3des.h"

#define LENGTH 16

using namespace std;

static const char *des_key = "~!@#$%^&*()_+QWE";

//assuming 16-byte src string
void do_encryption(string& file_to_be_encrypt, string& encrypted_file)
{
    for (unsigned i = 0; i < file_to_be_encrypt.length(); i += LENGTH)
    {
        std::string msg = file_to_be_encrypt.substr(i, LENGTH);
        unsigned char * encrypt = new unsigned char[LENGTH]();
        des2key((unsigned char *)des_key, EN0);
        D2des((unsigned char*)msg.c_str(),encrypt);
        if(file_to_be_encrypt.length() - i < LENGTH)
        {
            string encrypt_string((char *)encrypt, file_to_be_encrypt.length() - i);
            encrypted_file += encrypt_string;
        }
        else
        {
            string encrypt_string((char *)encrypt, LENGTH);
            encrypted_file += encrypt_string;
        }
    }
}

void do_decryption(string& file_to_be_decrpt, string& decrpted_file)
{
    string decrypted_message;

    for (unsigned i = 0; i < file_to_be_decrpt.length(); i += LENGTH)
    {
        std::string content = file_to_be_decrpt.substr(i, LENGTH);

        unsigned char *decrypt = new unsigned char[LENGTH]();

        unsigned char *new_string = (unsigned char *)content.c_str();

        des2key((unsigned char *)des_key, DE1);
        D2des(new_string,decrypt);

        if(file_to_be_decrpt.length() - i < LENGTH)
        {
            string decrypt_string((char *)decrypt, file_to_be_decrpt.length() - i);
            decrpted_file += decrypt_string;
        }
        else
        {
            string decrypt_string((char *)decrypt, LENGTH);
            decrpted_file += decrypt_string;
        }
    }
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