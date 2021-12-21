#ifndef _LIB_USER_SYSCALL_H_
#define _LIB_USER_SYSCALL_H_

#include "stdint.h"

enum SYSCALL_NR {
    SYS_GETPID = 0,
    SYS_WRITE
};

uint32_t get_pid(void);
uint32_t write(char* str);


#endif // _LIB_USER_SYSCALL_H_
