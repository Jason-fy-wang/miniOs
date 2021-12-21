#ifndef _LIB_USER_SYSCALL_H_
#define _LIB_USER_SYSCALL_H_

#include "stdint.h"

enum SYSCALL_NR {
    SYS_GETPID = 0,
    SYS_WRITE,
    SYS_MALLOC,
    SYS_FREE
};

uint32_t get_pid(void);
uint32_t write(char* str);
void free(void* ptr);
void* malloc(uint32_t size);

#endif // _LIB_USER_SYSCALL_H_
