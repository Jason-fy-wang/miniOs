#include "print.h"
#include "init.h"
#include "debug.h"
#include "thread.h"
#include "interrupt.h"
#include "console.h"
#include "process.h"
#include "syscall_init.h"
#include "syscall.h"
#include "stdio.h"
#include "fs.h"
#include "dir.h"
#include "fork.h"
#include "shell.h"

void init(void);


int main(void){
    put_str("i am kernel\n");
    init_all();
    cls_screen();
    console_put_str("[jason@localhost /] $ ");
    while (1);
    return 0;
}

void init(void){
    uint32_t ret_pid = fork();
    if(ret_pid){    // 父进程
        //printf("i am father, my pid is %d, child pid is %d\n", get_pid(), ret_pid);
        while(1);
    }else { // 子进程
        //printf("i am child, my pid is %d, ret_pid is %d\n", get_pid(), ret_pid);
        my_shell();
    }
    PANIC("init: should not be here.");
}