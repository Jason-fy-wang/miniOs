#include "thread.h"
#include "print.h"
#include "syscall.h"
#include "syscall_init.h"
#include "console.h"
#include "string.h"

syscall syscall_table[syscall_nr];

// 返回当前任务的pid
uint32_t sys_getpid(void){
    return running_thread()->pid;
}

uint32_t sys_write(char* str){
    console_put_str(str);
    return strlen(str);
}


void syscall_init(void){
    put_str("syscall init start\n");
    syscall_table[SYS_GETPID] = sys_getpid;
    syscall_table[SYS_WRITE] = sys_write;
    put_str("syscall init end\n");
}

