#include "print.h"
#include "init.h"
#include "debug.h"
#include "memory.h"
#include "thread.h"
#include "interrupt.h"
#include "console.h"
#include "ioqueue.h"
#include "keyboard.h"

void k_thread_a(void *);
void k_thread_b(void*);

int main(void){
    put_str("i am kernel\n");
    init_all();

    thread_start("threadA", 31, k_thread_a,"A_");
    thread_start("threadB", 8, k_thread_b,"B_");

    //打开中断
    intr_enable();
    while (1);
    return 0;
}

void k_thread_a(void *arg_){
    char * arg = arg_;
    while(1){
        enum intr_status old_status = intr_disable();
        if(!ioq_empty(&kbd_buf)){
            console_put_str(arg);
            char byte = ioq_getchar(&kbd_buf);
            console_put_char(byte);
        }
        intr_set_status(old_status);
    }
}

void k_thread_b(void* arg_){
    char *arg = arg_;

    while(1){
        enum intr_status old_status = intr_disable();
        if(!ioq_empty(&kbd_buf)){
            console_put_str(arg);
            char byte = ioq_getchar(&kbd_buf);
            console_put_char(byte);
        }
        intr_set_status(old_status);
    }
}