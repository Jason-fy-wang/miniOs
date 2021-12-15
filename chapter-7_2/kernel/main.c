#include "print.h"
#include "init.h"

int main(void){
    put_str("i am kernel\n");
    init_all();
    asm volatile("sti");    // 打开中断
    while (1);
    return 0;
}
