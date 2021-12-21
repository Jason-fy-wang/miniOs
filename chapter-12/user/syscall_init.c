#include "thread.h"
#include "print.h"
#include "syscall.h"
#include "syscall_init.h"

syscall syscall_table[syscall_nr];

// 返回当前任务的pid
uint32_t sys_getpid(void){
    return running_thread()->pid;
}


void syscall_init(void){
    put_str("syscall init start\n");
    syscall_table[SYS_GETPID] = sys_getpid;
    put_str("syscall init end\n");
}

