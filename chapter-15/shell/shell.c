#include "shell.h"
#include "stdio.h"
#include "debug.h"
#include "stdint.h"
#include "syscall.h"
#include "file.h"
#include "string.h"

#define cmd_len 128     // 最大支持键入128个字符的命令输入
#define MAX_ARG_NR 16   // 加上命令名外, 最多支持15个参数

// 存储输入的命令
static char cmd_line[cmd_len] = {0};

// 用来记录当前目录,是当前目录的缓存,每次执行命令时会更新此内容
char cwd_cache[64] = {0};

// 提示符
void print_prompt(void){
    printf("[jason@localhost %s]$", cwd_cache);
}

// 从键盘缓冲区中最大键入count个字符到buf
static void readline(char* buf, int32_t count){
    ASSERT(buf != NULL && count > 0);
    char* pos = buf;
    while(read(stdin_no, pos, 1) != -1 && (pos - buf) < count){
        switch(*pos){
            case '\n':
            case '\r':
                *pos = 0;       // 添加cmd_line的终止字符 0
                putchar('\n');
                return ;
            case '\b':
                if(buf[0] != '\b'){ // 阻止删除非本次输入的信息
                    --pos;          // 退回到缓冲区cmd_line中上一个字符
                    putchar('\b');
                }
                break;
            default:
                putchar(*pos);
                pos++;
        }
    }
    printf("readline: can't find entry_key in the cmd_line, max num of char is 128.\n");
}


// shell
void my_shell(void){
    cwd_cache[0] = '/';
    while(1){
        print_prompt();
        memset(cmd_line, 0, cmd_len);
        readline(cmd_line, cmd_len);
        if(cmd_line[0] == 0) {      // 只键入一个回车
            continue;
        }
    }
    PANIC("my_shell: should not be here.");
}
