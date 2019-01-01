//
// Created by czx on 19-1-1.
//

#ifndef GFW_GDB_PRINT_H
#define GFW_GDB_PRINT_H
#include <assert.h>



#ifdef  __cplusplus
extern "C" {
#endif
typedef enum {
    DGLOG_LOW,
    DGLOG_DBG,
    DGLOG_ERR,
    DGLOG_FATEL
} DgLogLevel;

#define    DGLOG_LVL_LOW   1
#define    DGLOG_LVL_DBG   2
#define    DGLOG_LVL_ERR   3
#define    DGLOG_LVL_FATEL  4

#define    DG_USE_DUMP      1
#define    DG_USE_LOG       1

#define DG_LOG_LEVEL  DGLOG_LVL_ERR
void dg_log(const char *fmt, ...);
void dg_dump(char *mem, unsigned long len);




#if DG_USE_LOG
#define DGLOG(lvl, fmt, ...) \
    do {\
        if((lvl) >= DG_LOG_LEVEL) {\
            dg_log("%s(%d): " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);\
        }\
    } while(0)
#else
#define DGLOG(...)
#endif
#if DG_USE_DUMP
#define DGDUMP(mem, len, fmt, ...) \
    do{\
        DGLOG(DGLOG_LVL_DBG, fmt, ##__VA_ARGS__);\
        dg_dump((char *)mem, len);\
    } while(0)
#else
#define DGDUMP(...)
#endif

#define DGLOW(fmt, ...) DGLOG(DGLOG_LVL_LOW, "LOW | " fmt, ##__VA_ARGS__)
#define DGDBG(fmt, ...) DGLOG(DGLOG_LVL_DBG, "DBG | " fmt, ##__VA_ARGS__)
#define DGERR(fmt, ...) DGLOG(DGLOG_LVL_ERR, "ERR | " fmt, ##__VA_ARGS__)
#define DGFAT(fmt, ...) DGLOG(DGLOG_LVL_FATEL, "FAT | " fmt, ##__VA_ARGS__)

#define ENSURE(expr)  \
    do{\
        if (!(expr)) {\
            DGFAT("Assert: " #expr);\
            assert(expr);\
            while(1);\
        }\
    }while(0)


#ifdef  __cplusplus
};
#endif
#endif //GFW_GDB_PRINT_H
