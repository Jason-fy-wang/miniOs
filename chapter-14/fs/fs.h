#ifndef _FS_FS_H_
#define _FS_FS_H_


#define MAX_FILES_PER_PART   4096          // 每个分区支持最大创建的文件数
#define BITS_PER_SECTOR     4096        // 每扇区的位数
#define SECTOR_SIZE         512         //  扇区字节数
#define BLOCK_SIZE      SECTOR_SIZE     // 块字节数


enum file_types{
    FT_UNKNOWN,             // 不支持的类型
    FT_REGULAR,             // 普通文件
    FT_DIRECTORY            // 目录
};

void filesys_init(void);

#endif // _FS_FS_H_

