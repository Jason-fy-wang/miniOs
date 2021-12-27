#ifndef _FS_INODE_H_
#define _FS_INODE_H_

#include "stdint.h"
#include "list.h"
#include "ide.h"

// inode结构
struct inode{
    uint32_t i_no;        // inode编号
    // 当此inode是文件时,i_size用来指定文件大小,若此iNode是目录, i_size是指该目录下所以目录项大小之和
    uint32_t i_size;

    uint32_t i_open_cnts;       // 记录此文件被打开的次数
    bool write_deny;        // 写文件不能并行, 进程写文件前检查此标识

    // i_sectors[0-11]是直接块, i_sectors[12]用来存储一级间接指针
    uint32_t i_sectors[13];

    struct list_elem inode_tag;
};

void inode_init(uint32_t inode_no,  struct inode* new_inode);
void inode_sync(struct partition* part, struct inode* inode, void* io_buf);
struct inode* inode_open(struct partition* part, uint32_t inode_no);
void inode_close(struct inode* inode);

void inode_delete(struct partition* part, uint32_t inode_no, void* io_buf);

void inode_release(struct partition* part, uint32_t inode_no);

#endif // _FS_INODE_H_
