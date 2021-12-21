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


void k_thread_a(void *);
void k_thread_b(void*);
void u_prog_a(void);
void u_prog_b(void);

int prog_a_pid=0, prog_b_pid=0;

int main(void){
    put_str("i am kernel\n");
    init_all();

    process_execute(u_prog_a, "user_a");
    process_execute(u_prog_b, "user_b");

    //打开中断
    intr_enable();
    console_put_str(" main_pid:0x");
    console_put_int(sys_getpid());
    console_put_char('\n');
    thread_start("threadA", 31, k_thread_a,"A_");
    thread_start("threadB", 8, k_thread_b,"B_");
    while (1);
    return 0;
}

void k_thread_a(void *arg_){
    char * arg = arg_;
    console_put_str(" thread_a_pid:0x");
    console_put_int(sys_getpid());
    console_put_char('\n');
    console_put_str(" prog_a_pid:0x");
    console_put_int(prog_a_pid);
    while(1);
}

void k_thread_b(void* arg_){
    char *arg = arg_;
    console_put_str(" thread_b_pid:0x");
    console_put_int(sys_getpid());
    console_put_char('\n');
    console_put_str(" prog_b_pid:0x");
    console_put_int(prog_b_pid);
    while(1);
}

void u_prog_a(void){
    char* name = "prog_a";
    printf(" I am %s, my pid is: 0x%x%c", name, get_pid(),'\n');
    while(1);
}
void u_prog_b(void){
    char* name = "prog_b";
    printf(" I am %s, my pid is: 0x%x%c", name, get_pid(),'\n');
    while(1);
}