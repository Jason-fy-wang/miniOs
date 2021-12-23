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
    thread_start("threadA", 31, k_thread_a,"A_");
    thread_start("threadB", 31, k_thread_b,"B_");

    sys_open("/file1", O_CREAT);
    while (1);
    return 0;
}

void k_thread_a(void *arg_){
    char * arg = arg_;
    void* addr1 = sys_malloc(256);
    void* addr2 = sys_malloc(255);
    void* addr3 = sys_malloc(254);
    console_put_str(" thread_a malloc addr:0x");
    console_put_int((int)addr1);
    console_put_char(',');
    console_put_int((int)addr2);
    console_put_char(',');
    console_put_int((int)addr3);
    console_put_char('\n');
    int cpu_delay = 100000;
    while(cpu_delay-- >0);
    sys_free(addr1);
    sys_free(addr2);
    sys_free(addr3);
    console_put_str(" thread_a end\n");
    while(1);
}


void k_thread_b(void* arg_){
    char *arg = arg_;
       void* addr1 = sys_malloc(256);
    void* addr2 = sys_malloc(255);
    void* addr3 = sys_malloc(254);
    console_put_str(" thread_b malloc addr:0x");
    console_put_int((int)addr1);
    console_put_char(',');
    console_put_int((int)addr2);
    console_put_char(',');
    console_put_int((int)addr3);
    console_put_char('\n');
    int cpu_delay = 100000;
    while(cpu_delay-- >0);
    sys_free(addr1);
    sys_free(addr2);
    sys_free(addr3);
    console_put_str(" thread_b end\n");
    while(1);
}



void u_prog_a(void){
    char* name = "prog_a";
    printf(" I am %s, my pid is: 0x%x%c", name, get_pid(),'\n');
    void* addr1 = malloc(256);
    void* addr2 = malloc(255);
    void* addr3 = malloc(254);
    printf( "prog_a malloc addr: 0x%x, 0x%x, 0x%x\n", (int)addr1,(int)addr2,(int)addr3);
    int cpu_delay = 100000;
    while(cpu_delay-- >0);
    free(addr1);
    free(addr2);
    free(addr3);
    while(1);
}
void u_prog_b(void){
    char* name = "prog_b";
    printf(" I am %s, my pid is: 0x%x%c", name, get_pid(),'\n');
       void* addr1 = malloc(256);
    void* addr2 = malloc(255);
    void* addr3 = malloc(254);
    printf( "prog_b malloc addr: 0x%x, 0x%x, 0x%x\n", (int)addr1,(int)addr2,(int)addr3);
    int cpu_delay = 100000;
    while(cpu_delay-- >0);
    free(addr1);
    free(addr2);
    free(addr3);
    while(1);
}



void test_thread_a(){
        void* addr1;
    void* addr2;
    void* addr3;
    void* addr4;
    void* addr5;
    void* addr6;
    void* addr7;
    console_put_str("  thread_a_start\n");
    int max = 1000;
    while(max-- > 0){
        int size  = 128;
        addr1 = sys_malloc(size);
        size *= 2;
        addr2 = sys_malloc(size);
        size *= 2;
        addr3 = sys_malloc(size);
        sys_free(addr1);
        addr4 = sys_malloc(size);
        size *= 2;
        size *= 2;
        size *= 2;
        size *= 2;
        size *= 2;
        size *= 2;
        size *= 2;
        addr5 = sys_malloc(size);
        addr6 = sys_malloc(size);
        sys_free(addr5);
        size *= 2;

        addr7 = sys_malloc(size);
        console_put_str("addr7=");
        console_put_int((int)addr7);
        sys_free(addr6);
        sys_free(addr7);
        sys_free(addr2);
        sys_free(addr3);
        sys_free(addr4);
    }
}


void test_thread_b(){
     void* addr1;
    void* addr2;
    void* addr3;
    void* addr4;
    void* addr5;
    void* addr6;
    void* addr7;
    void* addr8;
    void* addr9;
    console_put_str("  thread_b_start\n");
    int max = 1000;
    while(max-- > 0){
        int size  = 9;
        addr1 = sys_malloc(size);
        size *= 2;
        addr2 = sys_malloc(size);
        size *= 2;
        sys_free(addr2);
        addr3 = sys_malloc(size);
        sys_free(addr1);
        addr4 = sys_malloc(size);
        addr5 = sys_malloc(size);
        addr6 = sys_malloc(size);
        sys_free(addr5);
        size *= 2;
        addr7 = sys_malloc(size);
        sys_free(addr6);
        sys_free(addr7);
        sys_free(addr3);
        sys_free(addr4);
        size *= 2;
        size *= 2;
        size *= 2;
        addr1 = sys_malloc(size);
        addr2 = sys_malloc(size);
        addr3 = sys_malloc(size);
        addr4 = sys_malloc(size);
        addr5 = sys_malloc(size);
        addr6 = sys_malloc(size);
        addr7 = sys_malloc(size);
        addr8 = sys_malloc(size);
        addr9 = sys_malloc(size);
        console_put_str("addr9=");
        console_put_int((int)addr9);
        sys_free(addr1);
        sys_free(addr2);
        sys_free(addr3);
        sys_free(addr4);
        sys_free(addr5);
        sys_free(addr6);
        sys_free(addr7);
        sys_free(addr8);
        sys_free(addr9);
    }
}
