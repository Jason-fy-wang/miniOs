#ifndef _FS_FS_H_
#define _FS_FS_H_

#include "stdint.h"
#include "ide.h"
#include "string.h"

#define MAX_FILES_PER_PART   4096          // 每个分区支持最大创建的文件数
#define BITS_PER_SECTOR     4096        // 每扇区的位数
#define SECTOR_SIZE         512         //  扇区字节数
#define BLOCK_SIZE      SECTOR_SIZE     // 块字节数
#define MAX_PATH_LENGTH     512     // 最大路径长度

enum file_types{
    FT_UNKNOWN,             // 不支持的类型
    FT_REGULAR,             // 普通文件
    FT_DIRECTORY            // 目录
};

// 打开文件的选项
enum oflags {
    O_RDONLY,        // 只读
    O_WRONLY,       // 只写
    O_RDWR,         // 读写
    O_CREAT = 4     //
};

// 用来记录查找文件过程中已找到的上级路径, 也就是查找文件过程中 '走过的地址'
struct path_search_record{
    char searched_path[MAX_PATH_LENGTH];        //查找过程中的父路径
    struct dir* parent_dir;         // 文件或目录所在的直接父目录
    enum file_types file_type;          // 找到的文件类型
};


extern struct partition* cur_part;

void filesys_init(void);

int32_t  sys_open(const char* pathname, uint8_t flags);

int32_t path_depth_cnt(char* pathname);

#endif // _FS_FS_H_

