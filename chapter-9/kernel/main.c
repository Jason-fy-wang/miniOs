#include "print.h"
#include "init.h"
#include "debug.h"
#include "memory.h"
#include "thread.h"

void k_thread_a(void *);

int main(void){
    put_str("i am kernel\n");
    init_all();

    void* vaddr = get_kernel_pages(3);
    put_str("\n get_kernel_page start vaddr is : ");
    put_int((uint32_t)(vaddr));
    put_str("\n");
    thread_start("threadA", 31, k_thread_a,"argA");
    while (1);
    return 0;
}

void k_thread_a(void *arg_){
    char * arg = arg_;
    while(1){
        put_str(arg);
    }
}