#include "buildin_cmd.h"
#include "assert.h"
#include "string.h"
#include "dir.h"
#include "file.h"
#include "fs.h"
#include "syscall.h"

// 将路径old_abs_path中的..和.转换为实际路径后存入new_abs_path
static void wash_path(char* old_abs_path, char* new_abs_path){
    assert(old_abs_path[0] == '/');
    char name[MAX_FILE_NAME_LEN] = {0};
    char* sub_path = old_abs_path;
    sub_path = path_parse(sub_path, name);

    if(name[0] == '/'){ // 若只是键入了 '/',直接将 / 存入 new_abs_path
        new_abs_path[0] = '/';
        new_abs_path[1] = 0;
        return;
    }

    new_abs_path[0] = 0;
    strcat(new_abs_path, "/");
    while(name[0]) {
        if(!strcmp("..", name)) {
            char* slash_ptr = strrchr(new_abs_path, '/');
            if(slash_ptr != new_abs_path){
                // 如果new_abs_path为 "/a/b", '..'之后变为  /a
                *slash_ptr = 0;
            }else {
                // 如果new_abs_path为 /a, '..' 之后变为 /
                *(slash_ptr+1) = 0;
            }
            // 路径不是 .
        }else if(strcmp(".", name)){
            if(strcmp(new_abs_path, "/")){
                strcat(new_abs_path, "/");
            }
            strcat(new_abs_path, name);
        }
        memset(name, 0, MAX_FILE_NAME_LEN);
        if(sub_path){
            sub_path = path_parse(sub_path, name);
        }
    }
}

// 将path处理成不含 .. 和 . 的绝对路径, 存储在final_path
void make_clear_abs_path(char* path, char* final_path){
    char abs_path[MAX_PATH_LENGTH] = {0};

    // 先判断是否输入的是绝对路径
    if(path[0] != '/'){     // 输入的不是绝对路径,则拼接成绝对路径
        memset(abs_path, 0, MAX_PATH_LENGTH);
        if(getcwd(abs_path, MAX_PATH_LENGTH) != NULL) {
            if(!((abs_path[0] == '/') && abs_path[1] == 0)){
                // 表示abs_path表示的当前目录 不是根目录
                strcat(abs_path, "/");
            }
        }
    }
    strcat(abs_path, path);
    wash_path(abs_path, final_path);
}
