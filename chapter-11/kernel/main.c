#include "print.h"
#include "init.h"
#include "debug.h"
#include "thread.h"
#include "interrupt.h"
#include "console.h"
#include "process.h"

void k_thread_a(void *);
void k_thread_b(void*);
void u_prog_a(void);
void u_prog_b(void);

int test_var_a=0, test_var_b=0;

int main(void){
    put_str("i am kernel\n");
    init_all();

    thread_start("threadA", 31, k_thread_a,"A_");
    thread_start("threadB", 8, k_thread_b,"B_");

    process_execute(u_prog_a, "user_a");
    process_execute(u_prog_b, "user_b");

    //打开中断
    intr_enable();
    while (1);
    return 0;
}

void k_thread_a(void *arg_){
    char * arg = arg_;
    while(1){
        console_put_str("v_b: 0x");
        console_put_int(test_var_b);
    }
}

void k_thread_b(void* arg_){
    char *arg = arg_;

    while(1){
        console_put_str("v_b: 0x");
        console_put_int(test_var_b);
    }
}

void u_prog_a(void){
    while(1){
        test_var_a++;
    }
}
void u_prog_b(void){
    while (1){
        test_var_b++;
    }
}