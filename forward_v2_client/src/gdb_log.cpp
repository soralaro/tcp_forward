//
// Created by czx on 19-1-1.
//
#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <mutex>
#include <map>
#include "gdb_log.h"
static std::mutex mt;
void dg_log(const char *fmt, ...) {

    mt.lock();
    va_list pvar;
    va_start (pvar, fmt);
    vprintf (fmt, pvar);
    va_end (pvar);
    mt.unlock();
}
#define BYTE_PER_LINE  16
void dg_dump(char *mem, unsigned long len) {
    mt.lock();
    printf("============= dump start ============\n");
    for(unsigned long i = 0; i < len; i++) {
        if(i != 0 && i % BYTE_PER_LINE == 16) {
            printf("\n");
        }
        printf("%02X ", mem[i]);
    }
    printf("\n");
    printf("============= dump end ============\n");

    mt.unlock();
}