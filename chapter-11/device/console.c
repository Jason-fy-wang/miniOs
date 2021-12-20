#include "console.h"
#include "print.h"
#include "sync.h"
#include "thread.h"

static struct lock console_lock;

// 初始化终端
void console_init(void){
    lock_init(&console_lock);
}

// 获取终端
void console_acquire(void){
    lock_acquire(&console_lock);
}


void console_release(void){
    lock_release(&console_lock);
}

void console_put_str(const char* str){
    console_acquire();
    put_str(str);
    console_release();
}


void console_put_char(const uint8_t c){
    console_acquire();
    put_char(c);
    console_release();
}

void console_put_int(const uint32_t num){
    console_acquire();
    put_int(num);
    console_release();
}

