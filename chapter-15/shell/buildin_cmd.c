#include "buildin_cmd.h"
#include "assert.h"
#include "string.h"
#include "dir.h"
#include "file.h"
#include "fs.h"
#include "syscall.h"
#include "shell.h"
#include "stdio.h"

// 将路径old_abs_path中的..和.转换为实际路径后存入new_abs_path
static void wash_path(char* old_abs_path, char* new_abs_path){
    assert(old_abs_path[0] == '/');
    char name[MAX_FILE_NAME_LEN] = {0};
    char* sub_path = old_abs_path;
    sub_path = path_parse(sub_path, name);

    if(name[0] == 0){ // 若只是键入了 '/',直接将 / 存入 new_abs_path
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
            if(!((abs_path[0] == '/') && (abs_path[1] == 0))){
                // 表示abs_path表示的当前目录 不是根目录
                strcat(abs_path, "/");
            }
        }
    }
    strcat(abs_path, path);
    wash_path(abs_path, final_path);
}

// 内建命令

// pwd 命令
void buildin_pwd(uint32_t argc, char** argv UNUSED){
    if(argc != 1){
        printf("pwd: no argument support.\n");
        return;
    }else {
        if(NULL != getcwd(final_path, MAX_PATH_LENGTH)){
            printf("%s\n", final_path);
        }else {
            printf("pwd: get current work directory failed.\n");
        }
    }
}


// cd命令
char* buildin_cd(uint32_t argc, char** argv){
    if(argc > 2){
        printf("cd: only support 1 argument!\n");
        return NULL;
    }

    // 若只是键入cd而无参数, 直接返回到根目录
    if(argc == 1){
        final_path[0] = '/';
        final_path[1] = 0;
    }else {
        make_clear_abs_path(argv[1], final_path);
    }

    if(chdir(final_path) == -1){
        printf("cd: no such directory %s\n", final_path);
        return NULL;
    }
    return final_path;
}

// ls命令
void buildin_ls(uint32_t argc, char** argv){
    char* pathname = NULL;
    struct stat file_stat;
    memset(&file_stat, 0, sizeof(struct stat));

    bool long_info = false;
    uint32_t arg_path_nr = 0;
    uint32_t arg_idx  = 1;
    while(arg_idx < argc){  // 跨过argv[0], argv[0] 是ls
        if(argv[arg_idx][0] == '-'){        // 如果是选项, 单词首字符是-
            if(!strcmp("-l", argv[arg_idx])){       // 参数-l
                long_info = true;
            }else if(!strcmp("-h", argv[arg_idx])){     // 参数 -h
                printf("usage: -l list all information about the file.\n -h for help\n list all files in the current directory if no option.\n");
                return;
            }else {     // 目前只支持 -l -h 两个选项
                printf("ls: invalid option: %s\n Try 'ls -h' for more information.\n", argv[arg_idx]);
                return;
            }
        }else { // ls 的路径参数
            if(arg_path_nr == 0){
                pathname = argv[arg_idx];
                arg_path_nr = 1;
            }else {
                printf("ls: only support one path.\n");
                return ;
            }
        }
        arg_idx++;
    }
    if(pathname == NULL){   // 只输入了ls 或 ls -l,则以当前路径的绝对路径为参数
        if(NULL != getcwd(final_path, MAX_PATH_LENGTH)){
            pathname = final_path;
        }else {
            printf("ls: getcwd for default path failed.\n");
            return;
        }
    }else {
        make_clear_abs_path(pathname, final_path);
        pathname = final_path;
    }

    if(stat(pathname, &file_stat) == -1){
        printf("ls:cannot access %s: No such file or directory.\n", pathname);
        return;
    }

    if(file_stat.st_filetype == FT_DIRECTORY){  // 目录
        struct dir* dir = opendir(pathname);
        struct dir_entry* dir_e = NULL;
        char sub_pathname[MAX_PATH_LENGTH] = {0};
        uint32_t pathname_len = strlen(pathname);
        uint32_t last_char_idx = pathname_len - 1;
        memcpy(sub_pathname, pathname, pathname_len);
        if(sub_pathname[last_char_idx] != '/'){
            sub_pathname[pathname_len] = '/';
            pathname_len++;
        }
        rewinddir(dir);
        if(long_info){
            char ftype;
            printf("total: %d\n", file_stat.st_size);
            while((dir_e = readdir(dir))){
                ftype = 'd';
                if(dir_e->f_type == FT_REGULAR){
                    ftype='-';
                }
                sub_pathname[pathname_len] = 0;
                strcat(sub_pathname, dir_e->filename);
                memset(&file_stat, 0, sizeof(struct stat));
                if(stat(sub_pathname,&file_stat) == -1){
                    printf("ls: cannot access %s: No such file or directory.\n", dir_e->filename);
                    return;
                }
                printf("%c  %d  %d  %s\n", ftype, dir_e->i_no, file_stat.st_size, dir_e->filename);
            }
        }else {
            while((dir_e = readdir(dir))){
                printf("%s ", dir_e->filename);
            }
            printf("\n");
        }
        closedir(dir);
    }else { // 文件
        if(long_info){
            printf("-   %d  %d  %s\n", file_stat.st_ino, file_stat.st_size, pathname);
        }else {
            printf("%s\n", pathname);
        }
    }

}

// ps命令
void buildin_ps(uint32_t argc, char** argv UNUSED){
    if(argc != 1){
        printf("ps: no argument support.\n");
        return;
    }
    ps();
}

// clear命令
void buildin_clear(uint32_t argc, char** argv){
    if(argc != 1){
        printf("clear: no argument support.\n");
        return ;
    }
    clear();
}

// mkdir
int32_t buildin_mkdir(uint32_t argc, char** argv){
    int32_t ret = -1;

    if(argc != 2){
        printf("mkdir: only support one argument.\n");
    }else {
        make_clear_abs_path(argv[1], final_path);
        //若创建的不是根目录
        if(strcmp("/", final_path)){
            if(mkdir(final_path) == 0){
                ret = 0;
            }else {
                printf("mkdir: create directory %s failed.\n", argv[1]);
            }
        }
    }
    return ret;
}

// rmdir 命令
int32_t buildin_rmdir(uint32_t argc, char** argv){
    int32_t ret = -1;
    if(argc != 2){
        printf("rmdir: only support one argument.\n");
    }else {
        make_clear_abs_path(argv[1],final_path);
        if(strcmp("/", final_path)){
            if(rmdir(final_path) == 0){
                ret = 0;
            }else {
                printf("rmdir: remove %s failed.\n", argv[1]);
            }
        }
    }

    return ret;
}

// rm 命令
int32_t buildin_rm(uint32_t argc, char** argv){
    int32_t ret = -1;
    if(argc != 2){
        printf("rm: only support one argument.\n");
    }else {
        make_clear_abs_path(argv[1], final_path);
        if(strcmp("/", final_path)){
            if(unlink(final_path) == 0){
                ret = 0;
            }else {
                printf("rm: delete  %s failed.\n",argv[1]);
            }
        }
    }

    return ret;
}

