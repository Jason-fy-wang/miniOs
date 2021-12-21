#ifndef _LIB_USER_SYSCALL_H_
#define _LIB_USER_SYSCALL_H_

#include "stdint.h"

enum SYSCALL_NR {
    SYS_GETPID = 0
};

uint32_t get_pid(void);



#endif // _LIB_USER_SYSCALL_H_
