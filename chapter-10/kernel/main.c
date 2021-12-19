#include "print.h"
#include "init.h"
#include "debug.h"
#include "memory.h"
#include "thread.h"
#include "interrupt.h"
#include "console.h"

void k_thread_a(void *);
void k_thread_b(void*);

int main(void){
    put_str("i am kernel\n");
    init_all();

    //thread_start("threadA", 31, k_thread_a,"argA");
    //thread_start("threadB", 8, k_thread_b,"argB");

    //打开中断
    intr_enable();
    while (1);
    /*{
        console_put_str("Main ");
    }*/
    return 0;
}

void k_thread_a(void *arg_){
    char * arg = arg_;
    while(1){
        console_put_str(arg);
    }
}

void k_thread_b(void* arg_){
    char *arg = arg_;

    while(1){
        console_put_str(arg);
    }
}