#ifndef _LIB_USER_SYSCALL_H_
#define _LIB_USER_SYSCALL_H_

#include "stdint.h"
#include "thread.h"

enum SYSCALL_NR {
    SYS_GETPID = 0,
    SYS_WRITE,
    SYS_MALLOC,
    SYS_FREE,
    SYS_FORK
};

uint32_t get_pid(void);
uint32_t write(uint32_t fd, const char* buf, uint32_t count);
void free(void* ptr);
void* malloc(uint32_t size);
pid_t fork(void);


#endif // _LIB_USER_SYSCALL_H_
