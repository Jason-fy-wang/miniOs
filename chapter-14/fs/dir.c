#include "dir.h"
#include "ide.h"
#include "global.h"
#include "fs.h"
#include "inode.h"
#include "stdio_kernel.h"
#include "memory.h"
#include "string.h"
#include "debug.h"
#include "file.h"
#include "super_block.h"


struct dir root_dir;

// 打开根目录
void open_root_dir(struct partition* part){
    root_dir.inode = inode_open(part, part->sb->root_inode_no);
    root_dir.dir_pos = 0;
}

//  在分区part上打开i结点为inode_no的目录并返回目录指针
struct dir* dir_open(struct partition* part, uint32_t inode_no){
    struct dir* pdir = (struct dir*)sys_malloc(sizeof(struct dir));

    pdir->inode = inode_open(part, inode_no);
    pdir->dir_pos = 0;
    return pdir;
}

// 在part分区内的pdir目录内寻找名为name的文件或目录
// 找到后返回ture 并将其目录项 存入 dir_e中, 否则返回false
bool search_dir_entry(struct partition* part, struct dir* pdir, const char* name, struct dir_entry* dir_e){
    uint32_t  block_cnt = 140;      // 12个字节块 + 128个一级间接块=140块

    //  12个直接看和128间接块, 共560字节
    uint32_t* all_blocks = (uint32_t*)sys_malloc(48 + 512);
    if(all_blocks == NULL){
        printk("search_dir_entry: sys_malloc failed\n");
        return false;
    }

    uint32_t block_idx = 0;
    while(block_idx < 12){
        all_blocks[block_idx] = pdir->inode->i_sectors[block_idx];
        block_idx++;
    }

    block_idx = 0;

    if(pdir->inode->i_sectors[12] != 0){    // 存在一级间接块表
        ide_read(part->my_disk, pdir->inode->i_sectors[12], all_blocks+12, 1);
    }
    // 到此all_block存储的是该文件或目录的所有扇区地址

    // 写目录项的时候已经保证目录项不跨扇区,这样读写目录项是容易处理, 只申请容纳1个扇区的内存
    uint8_t* buf = (uint8_t*)sys_malloc(SECTOR_SIZE);

    struct dir_entry* p_de = (struct dir_entry*) buf;
    uint32_t dir_entry_size = part->sb->dir_entry_size;
    // 1一个扇区可容纳的目录项个数
    uint32_t dir_entry_cnt = SECTOR_SIZE / dir_entry_size;

    // 开始在所有块中查找目录项
    while(block_idx < block_cnt){
        // 块地址为0, 表示此块中无数据,继续查找其他块
        if(all_blocks[block_idx] == 0){
            block_idx++;
            continue;
        }

        ide_read(part->my_disk, all_blocks[block_idx], buf, 1);
        uint32_t dir_entry_idx = 0;

        while(dir_entry_idx < dir_entry_cnt){
            // 找到则返回
            if(!strcmp(p_de->filename, name)){
                memcpy(dir_e, p_de, dir_entry_size);
                sys_free(buf);
                sys_free(all_blocks);
                return true;
            }

            dir_entry_idx++;
            p_de++;
        }
        block_idx++;
        p_de = (struct dir_entry*)buf;
        memset(buf, 0, SECTOR_SIZE);
    }

    sys_free(buf);
    sys_free(all_blocks);
    return false;
}

//  关闭目录
void dir_close(struct dir* dir){
    /****** 根目录不能关闭
     * 1. 根目录子打开后就不应该被关闭, 否则话需要再次 open_root_dir
     * 2. root_dir 所在的内存是低端1M之内,并非在堆中,free会出问题
     */
     if(dir == &root_dir){
         return;
     }

     inode_close(dir->inode);
     sys_free(dir);
}


// 在内存中初始化目录项 p_de
void create_dir_entry(char* filename, uint32_t inode_no, uint8_t file_type, struct dir_entry* p_de){
    ASSERT(strlen(filename) < MAX_FILE_NAME_LEN);

    // 初始化目录项
    memcpy(p_de->filename, filename, strlen(filename));
    p_de->i_no = inode_no;
    p_de->f_type = file_type;
}


// 将目录项 p_de 写入到父目录 parent_dir中, io_buf由主调函数提供
bool sync_dir_entry(struct dir* parent_dir, struct dir_entry* p_de, void* io_buf){
    struct inode* dir_inode = parent_dir->inode;

    uint32_t dir_size = dir_inode->i_size;
    // 当前分区 dir 目录大小
    uint32_t dir_entry_size = cur_part->sb->dir_entry_size;

    ASSERT(dir_size % dir_entry_size == 0);

    // 每个扇区最大目录项数目
    uint32_t dir_entry_per_sec = (SECTOR_SIZE / dir_entry_size);


    int32_t block_lba = -1;

    // 将该目录项的所有扇区地址 存入all_block
    uint8_t block_idx = 0;
    uint32_t all_blocks[140] = {0};

    while(block_idx < 12){
        all_blocks[block_idx] = dir_inode->i_sectors[block_idx];
        block_idx++;
    }

    // 用于遍历目录项
    struct dir_entry* dir_e = (struct dir_entry*)io_buf;

    int32_t block_bitmap_idx = -1;

    // 开始遍历所有块以寻找目录项空位,若已有扇区中没有空闲位,在不超过文件大小的情况下
    // 申请新扇区来存储新的目录项
    block_idx = 0;
    while(block_idx < 140){
        block_bitmap_idx = -1;

        if(all_blocks[block_idx] == 0){
            block_lba = block_bitmap_alloc(cur_part);
            if(block_lba == -1){
                printk(" alloc block bitmap for sync_dir_entry failed.\n");
                return false;
            }

            // 每分配一个块就同步一次block_bitmap
            block_bitmap_idx = block_lba  - cur_part ->sb->data_start_lba;
            ASSERT(block_bitmap_idx != -1);
            bitmap_sync(cur_part, block_bitmap_idx, BLOCK_BITMAP);

            block_bitmap_idx = -1;
            if(block_idx < 12){
                dir_inode->i_sectors[block_idx] = all_blocks[block_idx] = block_lba;
            }else if(block_idx == 12){
                // 若是尚未分配一级间接块表
                // 将上面分配的块作为一级间接块表地址
                dir_inode->i_sectors[12] = block_lba;

                block_lba = -1;
                // 再分配一个块作为第0个间接块
                block_lba = block_bitmap_alloc(cur_part);
                if(block_lba == -1){
                    block_bitmap_idx = dir_inode->i_sectors[12] - cur_part->sb->data_start_lba;
                    bitmap_set(&cur_part->block_bitmap, block_bitmap_idx, 0);
                    dir_inode->i_sectors[12] = 0;
                    printk("alloc block bitmap for syn_dir_entry failed.\n");
                    return false;
                }
                // 每分配一次block_bitmap 就同步一次
                block_bitmap_idx = block_lba  - cur_part->sb->data_start_lba;
                ASSERT(block_bitmap_idx != -1);
                bitmap_sync(cur_part, block_bitmap_idx, BLOCK_BITMAP);

                all_blocks[12] = block_lba;
                // 把新分配的第0个间接地址 写入一级间接表
                ide_write(cur_part->my_disk, dir_inode->i_sectors[12], all_blocks+12, 1);
            }else {
                // 间接块已分配
                all_blocks[block_idx] = block_lba;

                // 把新分配的block_idx - 12 个间接表地址写入一级间接表
                ide_write(cur_part->my_disk, dir_inode->i_sectors[12], all_blocks+12, 1);
            }

            // 将新目录项p_de 写入新分配的间接块
            memset(io_buf, 0, SECTOR_SIZE);
            memcpy(io_buf, p_de, dir_entry_size);

            ide_write(cur_part->my_disk, all_blocks[block_idx], io_buf, 1);
            dir_inode->i_size += dir_entry_size;
            return true;
        }

        // block_idx块已经存在,将其读入内存,然后再该块中查找空白目录项
        ide_read(cur_part->my_disk, all_blocks[block_idx], io_buf, 1);

        // 在扇区中查找空白目录项
        uint8_t dir_entry_idx = 0;

        while(dir_entry_idx < dir_entry_per_sec){
            if((dir_e + dir_entry_idx)->f_type == FT_UNKNOWN){
                // FT_UNKNOWN为0, 无论是否初始化,或是删除文件后
                memcpy(dir_e+dir_entry_idx, p_de, dir_entry_size);

                ide_write(cur_part->my_disk, all_blocks[block_idx], io_buf, 1);

                dir_inode->i_size += dir_entry_size;
                return true;
            }
            dir_entry_idx++;
        }
        block_idx++;
    }
    printk("directory is full.\n");
    return false;
}

// 把分区part目录pdir中编号为 inode_no的目录项删除
bool delete_dir_entry(struct partition *part, struct dir* pdir, uint32_t inode_no, void* io_buf){

    struct inode* dir_inode = pdir->inode;

    uint32_t block_idx =0, all_blocks[140] = {0};
    // 收集全部目录地址
    while(block_idx < 12){
        all_blocks[block_idx] = dir_inode->i_sectors[block_idx];
        block_idx++;
    }
    if(dir_inode->i_sectors[12]){
        ide_read(part->my_disk, dir_inode->i_sectors[12], all_blocks+12, 1);
    }

    // 目录项存储时,保证不会跨扇区
    uint32_t dir_entry_size = part->sb->dir_entry_size;
    uint32_t dir_entrys_per_sec = (SECTOR_SIZE / dir_entry_size);

    // 每扇区最大的目录项数目
    struct dir_entry* dir_e = (struct dir_entry*)io_buf;
    struct dir_entry* dir_entry_found = NULL;
    uint8_t dir_entry_idx, dir_entry_cnt;

    bool is_dir_first_block = false;
    // 遍历所有块,寻找目录项
    block_idx = 0;
    while(block_idx < 140){
        is_dir_first_block = false;
        if(all_blocks[block_idx] == 0){
            block_idx++;
            continue;
        }

        dir_entry_idx = dir_entry_cnt = 0;
        memset(io_buf, 0, SECTOR_SIZE);
        // 读取扇区,获取目录项
        ide_read(part->my_disk, all_blocks[block_idx], io_buf, 1);

        // 遍历所有的目录项, 统计该扇区的目录项数量以及 是否有带删除的目录项
        while(dir_entry_idx < dir_entrys_per_sec){
            if((dir_e + dir_entry_idx)->f_type != FT_UNKNOWN){
                if(!strcmp((dir_e + dir_entry_idx)->filename,".")){
                    is_dir_first_block = true;
                }else if (strcmp((dir_e+ dir_entry_idx)->filename, ".") && 
                        strcmp((dir_e + dir_entry_idx)->filename, "..")) {
                            dir_entry_cnt++;
                            // 统计此扇区内的目录项个数,用来判断删除目录项后是否该回收该扇区
                            if((dir_e + dir_entry_idx)->i_no == inode_no){
                                ASSERT(dir_entry_found == NULL);
                                // 找到也继续遍历,统计总共的目录项数目
                                dir_entry_found = dir_e+dir_entry_idx;
                            }
                }
            }
            dir_entry_idx++;
        }

        // 此扇区没有找到,继续下个扇区找
        if(dir_entry_found == NULL){
            block_idx++;
            continue;
        }

        // 在此扇区找到目录项后,清除该目录项并判断是否回收扇区,随后直接退出循环
        ASSERT(dir_entry_cnt >= 1);
        // 除目录第一个扇区外, 若该扇区只有该目录项自己, 则将整个扇区回收
        if(dir_entry_cnt == 1 && !is_dir_first_block){
            // a. 在块位图中回收该块
            uint32_t block_bitmap_idx = all_blocks[block_idx] - part->sb->data_start_lba;

            bitmap_set(&part->block_bitmap, block_bitmap_idx, 0);
            bitmap_sync(part, block_bitmap_idx, BLOCK_BITMAP);

            // b. 将块地址从数组 i_sectors 或 索引表中去除
            if(block_idx < 12){
                dir_inode->i_sectors[block_idx] = 0;
            }else{// 在间接表中
                // 先判断间接表 中 间接块的数量,如果仅有1个间接块, 连同间接表所在的块一块回收
                uint32_t indirect_blocks = 0;
                uint32_t indirect_block_idx = 12;
                while(indirect_block_idx < 140){
                    if (all_blocks[indirect_block_idx] != 0){
                        indirect_blocks++;
                    }
                }
                ASSERT(indirect_blocks >= 1);

                // 间接索引表中还包括其他间接块, 仅在索引表中擦除当前这个间接块地址
                if(indirect_blocks > 1){
                    all_blocks[block_idx] = 0;
                    ide_write(part->my_disk, dir_inode->i_sectors[12], all_blocks+12, 1);
                // 间接索引表中就只有当前这个间接块
                }else {
                // 直接回收间接索引表所在块回收,并擦除间接索引块地址
                    block_bitmap_idx = dir_inode->i_sectors[12] - part->sb->data_start_lba;
                    bitmap_set(&part->block_bitmap, block_bitmap_idx, 0);
                    bitmap_sync(part, block_bitmap_idx, BLOCK_BITMAP);

                    //间接表地址清0
                    dir_inode->i_sectors[12] = 0;
                }
            }
        }else { // dir_entry_cnt != 1
            memset(dir_entry_found,0, dir_entry_size);
            ide_write(part->my_disk, all_blocks[block_idx], io_buf, 1);
        }

        // 更新inode 结点信息 并 同步到硬盘
        ASSERT(dir_inode->i_size >= dir_entry_size);
        dir_inode->i_size -= dir_entry_size;
        memset(io_buf,0, SECTOR_SIZE*2);
        inode_sync(part, dir_inode, io_buf);
        return true;
    }
    // 未找到
    return false;
}

