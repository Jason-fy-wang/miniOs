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

void init(void);


int main(void){
    put_str("i am kernel\n");
    init_all();

    //打开中断
    //intr_enable();

    while (1);
    return 0;
}

void init(void){
    uint32_t ret_pid = fork();
    if(ret_pid){
        printf("i am father, my pid is %d, child pid is %d\n", get_pid(), ret_pid);
    }else {
        printf("i am child, my pid is %d, ret_pid is %d\n", get_pid(), ret_pid);
    }
    while(1);
}