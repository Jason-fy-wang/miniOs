#include "print.h"
#include "init.h"
#include "debug.h"
#include "memory.h"

int main(void){
    put_str("i am kernel\n");
    init_all();

    void* vaddr = get_kernel_pages(3);
    put_str("\n get_kernel_page start vaddr is : ");
    put_int((uint32_t)(vaddr));
    put_str("\n");
    while (1);
    return 0;
}
