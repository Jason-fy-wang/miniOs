#include "keyboard.h"
#include "print.h"
#include "interrupt.h"
#include "io.h"
#include "global.h"

#define KBD_BUF_PORT 0x60   //键盘buffer寄存器端口号

// 键盘中断处理程序
static void intr_keyboard_handler(void){
    put_char('k');
    inb(KBD_BUF_PORT);
    return;
}

// 键盘初始化
void keyboard_init(void){
    put_str("keyboard init start \n");
    register_handler(0x21, intr_keyboard_handler);
    put_str("keyboard init done\n");
}

