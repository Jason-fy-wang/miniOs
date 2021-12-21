#ifndef _USER_SYSCALLL_INIT_H_
#define _USER_SYSCALLL_INIT_H_

#include "stdint.h"

#define syscall_nr 32
typedef void* syscall;

void syscall_init(void);
uint32_t sys_getpid(void);
extern syscall syscall_table[syscall_nr];

#endif // _USER_SYSCALLL_INIT_H_