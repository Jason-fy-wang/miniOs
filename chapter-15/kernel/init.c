#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "timer.h"
#include "memory.h"
#include "thread.h"
#include "console.h"
#include "keyboard.h"
#include "tss.h"
#include "syscall_init.h"
#include "ide.h"
#include "fs.h"

void init_all(){
    put_str("init_all \n");
    idt_init();             // 初始化中断
    timer_init();           // 初始化 PIT
    mem_init();             // 初始化内存管理系统
    thread_init();          // 初始化线程
    console_init();         // 初始化控制台
    keyboard_init();          // 键盘初始化
    tss_init();             // tss
    syscall_init();         // 系统调用
    intr_enable();    // 后面的ide_init需要打开中断
    ide_init();             // 硬盘
    filesys_init();         // 文件系统
}

