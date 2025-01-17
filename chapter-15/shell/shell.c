#include "shell.h"
#include "stdio.h"
#include "assert.h"
#include "stdint.h"
#include "syscall.h"
#include "file.h"
#include "string.h"
#include "buildin_cmd.h"


#define cmd_len 128     // 最大支持键入128个字符的命令输入
#define MAX_ARG_NR 16   // 加上命令名外, 最多支持15个参数

// 存储输入的命令
static char cmd_line[MAX_PATH_LENGTH] = {0};

char final_path[MAX_PATH_LENGTH] = {0};
// 用来记录当前目录,是当前目录的缓存,每次执行命令时会更新此内容
char cwd_cache[64] = {0};

// 提示符
void print_prompt(void){
    printf("[jason@localhost %s]$", cwd_cache);
}

// 从键盘缓冲区中最大键入count个字符到buf
static void readline(char* buf, int32_t count){
    assert(buf != NULL && count > 0);
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

            // ctrl+l 清屏
            case 'l'-'a':
                // 1.先将当前的字符 'l'- 'a' 设置为0
                *pos = 0;
                // 2.再将屏幕清空
                clear();
                // 3. 打印提示符
                print_prompt();
                // 4.将之前键入的内容再次打印
                printf("%s", buf);
                break;

            // ctrl_u 清掉输入
            case 'u'-'a':
                while(buf != pos){
                    putchar('\b');
                    *(pos--) = 0;
                }
                break;
            default:
                putchar(*pos);
                pos++;
        }
    }
    printf("readline: can't find entry_key in the cmd_line, max num of char is 128.\n");
}


// 分析字符串cmd_str中以token为分隔符的单词,将各单词的指针存入argv数组中
static int32_t cmd_parse(char* cmd_str, char** argv, char token){
    assert(cmd_str != NULL);
    int32_t arg_idx = 0;
    while(arg_idx < MAX_ARG_NR){
        argv[arg_idx] = NULL;
        arg_idx++;
    }

    char* next = cmd_str;
    int32_t argc = 0;
    // 外层循环处理整个命令行
    while(*next){
        // 去除命令字或参数之间的空格
        while(*next == token){
            next++;
        }

        // 处理最后一个参数后接空格的情况, 如 'ls dir2 '
        if(*next == 0){
            break;
        }

        argv[argc] = next;
        // 内层循环处理命令行中每个命令字及参数
        while(*next && *next != token){
            next++;
        }
        // 如果未结束(是token字符), 使token变为0
        if(*next){
            *next++ = 0;    // 将token替换为字符串结束符0,作为一个单词的结束,并将字符串指针next指向下一个字符
        }
        if(argc > MAX_ARG_NR){
            return -1;
        }
        argc++;
    }
    return argc;
}

char* argv[MAX_ARG_NR];
int32_t argc = -1;

void my_shell(void){
    cwd_cache[0] = '/';
    while(1){
        print_prompt();
        memset(final_path,0, MAX_PATH_LENGTH);
        memset(cmd_line, 0, MAX_PATH_LENGTH);
        readline(cmd_line, MAX_PATH_LENGTH);
        if(cmd_line[0] == 0) {      // 只键入一个回车
            continue;
        }
        argc = -1;
        argc = cmd_parse(cmd_line, argv, ' ');
        if(argc == -1){
            printf("num of arguments exceed %d\n", MAX_PATH_LENGTH);
            continue;
        }

        if(!strcmp("ls", argv[0])){
            buildin_ls(argc, argv);
        }else if(!strcmp("cd", argv[0])){
            if(buildin_cd(argc,argv) != NULL){
                memset(cwd_cache, 0, MAX_PATH_LENGTH);
                strcpy(cwd_cache, final_path);
            }
        }else if(!strcmp("pwd", argv[0])){
            buildin_pwd(argc,argv);
        }else if(!strcmp("ps", argv[0])){
            buildin_ps(argc, argv);
        }else if(!strcmp("clear", argv[0])){
            buildin_clear(argc, argv);
        }else if(!strcmp("mkdir", argv[0])){
            buildin_mkdir(argc, argv);
        }else if(!strcmp("rmdir", argv[0])){
            buildin_rmdir(argc, argv);
        }else if(!strcmp("rm", argv[0])){
            buildin_rm(argc, argv);
        }else {
            printf("external command.\n");
        }
    }
    panic("my_shell: should not be here.");
}

// shell
// void my_shell(void){
//     cwd_cache[0] = '/';
//     while(1){
//         print_prompt();
//         memset(cmd_line, 0, cmd_len);
//         readline(cmd_line, cmd_len);
//         if(cmd_line[0] == 0) {      // 只键入一个回车
//             continue;
//         }
//     }
//     panic("my_shell: should not be here.");
// }
