#include "file.h"
#include "stdio_kernel.h"
#include "thread.h"
#include "ide.h"
#include "fs.h"
#include "debug.h"
#include "dir.h"
#include "inode.h"
#include "super_block.h"
#include "interrupt.h"

struct file file_table[MAX_FILE_OPEN];

// 从文件表file_table中获取一个空闲为,成功返下标, 失败返回-1
int32_t  get_free_slot_in_global(void){

    uint32_t fd_idx = 3;
    while(fd_idx < MAX_FILE_OPEN){
        if(file_table[fd_idx].fd_inode == NULL){
            break;
        }
        fd_idx++;
    }
    if(fd_idx == MAX_FILE_OPEN){
        printk("exceed max open files\n");
        return -1;
    }
    return fd_idx;
}


//将全局描述符下标安装到进程 或 线程自己的文件描述符数组 fd_table中
// 成功返回下标, 失败返回-1
int32_t pcb_fd_install(int32_t global_fd_idx){
    struct task_struct* cur = running_thread();

    int8_t local_fd_idx = 3;    // 跨过0 1 2

    while(local_fd_idx < MAX_FILES_OPEN_PER_PROC){
        if(cur->fd_table[local_fd_idx] == -1) {      // -1 表示 free slot
            cur->fd_table[local_fd_idx] = global_fd_idx;
            break;
        }
        local_fd_idx++;

        if(local_fd_idx == MAX_FILES_OPEN_PER_PROC){
            printk("exceed max open files_per_proc\n");
            return -1;
        }
    }
    return local_fd_idx;
}

// 分配一个inode 并 返回 inode_no
int32_t inode_bitmap_alloc(struct partition* part){
    int32_t bit_idx  = bitmap_scan(&part->inode_bitmap, 1);
    if(bit_idx == -1){
        return -1;
    }

    bitmap_set(&part->inode_bitmap, bit_idx, 1);
    return bit_idx;
}

// 分配一个扇区, 返回其扇区地址
int32_t  block_bitmap_alloc(struct partition* part){
    int32_t bit_idx = bitmap_scan(&part->block_bitmap, 1);
    if(bit_idx == -1){
        return -1;
    }

    bitmap_set(&part->block_bitmap, bit_idx, 1);
    // 把inode_bitmap_malloc不同,此处返回的不是位图索引,而是具体可用扇区的地址
    return (part->sb->data_start_lba + bit_idx);
}


// 将内存中bitmap第bit_idx为所在的512字节同步到硬盘
void bitmap_sync(struct partition* part, uint32_t bit_idx, uint8_t btmp){
    // 此结点索引相对于位图的扇区偏移量
    uint32_t off_sec = bit_idx / 4096;

    // 此结点索引相对于位图的字节偏移量
    uint32_t off_size = off_sec * BLOCK_SIZE;

    uint32_t sec_lba;
    uint8_t* bitmap_off;
    // 需要被同步的位图一般只有 inode_bitmap和block_bitmap

    switch(btmp){
        case INODE_BITMAP:
            sec_lba = part->sb->inode_bitmap_lba + off_sec;
            bitmap_off = part->inode_bitmap.bits + off_size;
            break;
        case BLOCK_BITMAP:
            sec_lba = part->sb->block_bitmap_lba + off_sec;
            bitmap_off = part->block_bitmap.bits + off_size;
            break;
    }
    ide_write(part->my_disk, sec_lba, bitmap_off, 1);
}

// 创建文件,若成功则返回文件描述符, 否则返回-1
int32_t file_create(struct dir* parent_dir, char* filename, uint8_t flag){

    // 后续操作的缓冲区
    void* io_buf = sys_malloc(1024);
    if(io_buf == NULL){
        printk("in file_create: sys_malloc for io_buf failed.\n");
        return -1;
    }

    uint8_t rollback_step=0;    // 用于操作失败时回滚各种资源状态

    // 为新文件分配 inode
    int32_t inode_no = inode_bitmap_alloc(cur_part);
    if(inode_no == -1){
        printk("in fil_create: allocate inode failed.\n");
        return -1;
    }

    // 此inode要从堆中申请内存, 不可生成局部变量,因为file_table数组中的文件描述符inode要指向它
    struct inode* new_file_inode = (struct inode*)sys_malloc(sizeof(struct inode));

    if(new_file_inode == NULL){
        printk("file_create: sys_malloc for inode failed.\n");
        rollback_step = 1;

        goto rollback;
    }

    inode_init(inode_no, new_file_inode);

    //返回的是file_table数组的下标
    int fd_idx = get_free_slot_in_global();
    if(fd_idx == -1){
        printk("exceed max open files\n");
        rollback_step=2;
        goto rollback;
    }

    file_table[fd_idx].fd_inode = new_file_inode;
    file_table[fd_idx].fd_pos = 0;
    file_table[fd_idx].fd_flag = flag;
    file_table[fd_idx].fd_inode->write_deny = false;

    struct dir_entry new_dir_entry;
    memset(&new_dir_entry, 0, sizeof(struct dir_entry));

    create_dir_entry(filename, inode_no, FT_REGULAR, &new_dir_entry);

    // 同步内存数据到硬盘
    //1.在目录parent_dir下安装目录项 new_dir_entry
    if(!sync_dir_entry(parent_dir, &new_dir_entry, io_buf)){
        printk("sync dir_entry to disk failed.\n");
        rollback_step = 3;
        goto rollback;
    }

    memset(io_buf,0, 1024);
    // 2. 将父目录i节点的内容同步到硬盘
    inode_sync(cur_part, parent_dir->inode, io_buf);

    // 3. 将新创建文件的inode内容同步到硬盘
    memset(io_buf,0, 1024);
    inode_sync(cur_part, new_file_inode, io_buf);

    // 4. 将inode_bitmap同步到硬盘
    bitmap_sync(cur_part, inode_no, INODE_BITMAP);


    // 5. 将创建的文件 inode 添加到 open_inodes链表
    list_push(&cur_part->open_inodes, &new_file_inode->inode_tag);
    new_file_inode->i_open_cnts = 1;

    sys_free(io_buf);
    return pcb_fd_install(fd_idx);

rollback:
    switch(rollback_step){
        case 3:
            // 将file_table 中的相应位清空
            memset(&file_table[fd_idx], 0, sizeof(struct file));
        case 2:
            sys_free(new_file_inode);
        case 1:
            /******
             *如果新文件的inode结点创建失败,之前位图中分配的 inode_no 也要恢复
             */
            bitmap_set(&cur_part->inode_bitmap, inode_no, 0);
            break;
    }
    sys_free(io_buf);
    return -1;
}


// 打开编号为inode_no的inode对应的文件
int32_t file_open(uint32_t inode_no, uint8_t flags){
    int fd_idx = get_free_slot_in_global();
    if(fd_idx == -1){
        printk("exceed max open file.\n");
        return -1;
    }

    file_table[fd_idx].fd_inode = inode_open(cur_part, inode_no);

    // 每次打开文件,要将fd_pos还原为0, 即让文件内的指针指向开头
    file_table[fd_idx].fd_pos = 0;

    file_table[fd_idx].fd_flag = flags;
    bool* write_deny = &file_table[fd_idx].fd_inode->write_deny;

    if(flags & O_WRONLY || flags & O_CREAT) {
        // 关于写文件,判断是否有其他进程正在写此文件,若是读文件, 则无需考虑 write_deny
        enum intr_status  old_status = intr_disable();
        if(!(*write_deny)){
            *write_deny = true;
            intr_set_status(old_status);
        }else {
            // 有进程在写, 失败退出
            intr_set_status(old_status);
            printk("file can't be write now. try again later.\n");
            return -1;
        }
    }
    return pcb_fd_install(fd_idx);
}


// 关闭文件
int32_t file_close(struct file* file){
    if(file == NULL){
        return -1;
    }
    file->fd_inode->write_deny = false;
    inode_close(file->fd_inode);
    file->fd_inode = NULL;
    return 0;
}


// 把buf中的count个字节写入file,成功则返回写入的字节数, 失败则返回-1
int32_t  file_write(struct file* file, const void* buf, uint32_t count){

    if((file->fd_inode->i_size + count) > (BLOCK_SIZE * 140)){
        // 目前文件最大支持 512 * 140= 71680
        printk("exceed max file_size: 71680 bytes. write file failed.\n");
        return -1;
    }

    uint8_t* io_buf = sys_malloc(512);

    if(io_buf == NULL){
        printk("file_write: sya_malloc for io_buf failed.\n");
        return -1;
    }
    // 记录所有块地址
    uint32_t* all_blocks = (uint32_t*)sys_malloc(BLOCK_SIZE + 48);
    if(all_blocks == NULL){
        printk("file_write: sys_malloc for all_blocks failed.\n");
        return -1;
    }

    const uint8_t* src = buf;       // 待写入的数据
    uint32_t bytes_written = 0;    // 记录已写入的数据大小
    uint32_t size_left = count;     // 用来记录未写入的数据大小
    int32_t  block_lba = -1;       // 块地址
    uint32_t block_bitmap_idx = 0;    // 用来记录block对应的block_bitmap中的索引,作为参数传递给bitmap_sync

    uint32_t sec_idx;       // 用来索引扇区
    uint32_t sec_lba;       // 扇区地址
    uint32_t sec_off_bytes; // 扇区内字节偏移量
    uint32_t sec_left_bytes;    // 扇区内剩余字节量
    uint32_t chunk_size;    // 每次写入硬盘的数据块大小
    int32_t  indirect_block_table;  // 用来获取一级间接表地址
    uint32_t block_idx ;        // 块索引

    // 判断文件是否是第一次写入, 如果是,为其分配一个块
    if(file->fd_inode->i_sectors[0] == 0){
        block_lba = block_bitmap_alloc(cur_part);
        if(block_lba == -1){
            printk("file_write: block_bitmap_alloc failed.\n");
            return -1;
        }

        file->fd_inode->i_sectors[0] = block_lba;

        // 每分配一个块就同步到硬盘
        block_bitmap_idx = block_lba - cur_part->sb->data_start_lba;
        ASSERT(block_bitmap_idx != 0);
        bitmap_sync(cur_part, block_bitmap_idx, BLOCK_BITMAP);
    }

    // 写入count个字节前,改文件已经占用的块数
    uint32_t file_has_use_blocks = file->fd_inode->i_size / BLOCK_SIZE + 1;

    // 存储count个字节后,该文件将占用的块数
    uint32_t file_will_use_blocks = (file->fd_inode->i_size+count)  / BLOCK_SIZE + 1;

    ASSERT(file_will_use_blocks <= 140);
    // 通过此增量判断是否要分配扇区, 如增量为0, 表示原扇区够用
    uint32_t add_blocks = file_will_use_blocks - file_has_use_blocks;

    if(add_blocks == 0){
        // 在同意扇区内写入地址, 不涉及到分配新扇区
        if(file_will_use_blocks <= 12){
            block_idx = file_has_use_blocks -1;
            all_blocks[block_idx] = file->fd_inode->i_sectors[block_idx];
        }else {     // 为写入前已经占用了 间接块,需要将间接块读取进来
            ASSERT(file->fd_inode->i_sectors[12] != 0);
            indirect_block_table = file->fd_inode->i_sectors[12];
            ide_read(cur_part->my_disk, indirect_block_table, all_blocks+12, 1);
        }
    }else {     // add_block不为0, 表示有增量
        // 表示需要分配新扇区以及 是否分诶一级间接块表
        //1. 12个直接快够用
        if(file_will_use_blocks <= 12){
            // 先将有剩余控件的可继续用的扇区地址写入all_blocks
            block_idx = file_has_use_blocks - 1;
            ASSERT(file->fd_inode->i_sectors[block_idx]!= 0);
            all_blocks[block_idx] = file->fd_inode->i_sectors[block_idx];

            // 再将未来要用的扇区分配好后写入all_blocks
            block_idx = file_has_use_blocks;    // 第一个要分配的扇区
            while(block_idx < file_will_use_blocks){
                block_lba = block_bitmap_alloc(cur_part);
                if(block_lba == -1){
                    printk("file write: block_bitmap_alloc for situation 1 failed.\n");
                    return -1;
                }
                // 写文件时,不应该存在块未使用,但已经分配扇区的情况
                // 当文件被扇区时,就会把块地址清0
                // 确保尚未分配扇区地址
                ASSERT(file->fd_inode->i_sectors[block_idx] == 0);
                file->fd_inode->i_sectors[block_idx] = all_blocks[block_idx] = block_lba;

                // 同步位图到硬盘
                block_bitmap_idx = block_lba - cur_part->sb->data_start_lba;
                bitmap_sync(cur_part, block_bitmap_idx, BLOCK_BITMAP);

                block_idx++;    // 下一个新扇区
            }
            // 情况2: 旧数据在12个直接块内, 新数据将使用间接块
        }else if(file_has_use_blocks <= 12 && file_will_use_blocks > 12){
            // 现将剩余控件的可继续用的扇区收集到all_blocks
            block_idx = file_has_use_blocks - 1;
            all_blocks[block_idx] = file->fd_inode->i_sectors[block_idx];
            // 创建一级间接表
            block_lba = block_bitmap_alloc(cur_part);
            if(block_lba == -1){
                printk("file_Write: block_bitmap_alloc for situation2 failed.\n");
                return -1;
            }
            ASSERT(file->fd_inode->i_sectors[12] == 0);

            // 分配一级间接表
            indirect_block_table = file->fd_inode->i_sectors[12] = block_lba;

            // 第一个未使用的块, 即本文件最后一个已经使用的直接块的下一块
            block_idx = file_has_use_blocks;

            while(block_idx < file_will_use_blocks){
                block_lba = block_bitmap_alloc(cur_part);

                if(block_lba == -1){
                    printk("file_write: block_bitmap_alloc for situation 2-1 failed.\n");
                    return -1;
                }

                if(block_idx < 12){
                    ASSERT(file->fd_inode->i_sectors[block_idx] == 0);
                    file->fd_inode->i_sectors[block_idx] = all_blocks[block_idx] = block_lba;
                }else {
                    // 间接块只写入到all_blocks
                    all_blocks[block_idx] = block_lba;
                }
                // 位图同步
                block_bitmap_idx = block_lba - cur_part->sb->data_start_lba;
                bitmap_sync(cur_part, block_bitmap_idx, BLOCK_BITMAP);
                block_idx++;
            }
            // 写入一级间接表
            ide_write(cur_part->my_disk, indirect_block_table, all_blocks+12, 1);
            // 情况3: 新数据占据间接块
        }else if (file_has_use_blocks > 12) {
            ASSERT(file->fd_inode->i_sectors[12] != 0);
            indirect_block_table = file->fd_inode->i_sectors[12];
            //  获取间接表
            ide_read(cur_part->my_disk, indirect_block_table, all_blocks+12, 1);

            block_idx = file_has_use_blocks;

            while(block_idx < file_will_use_blocks){
                block_lba = block_bitmap_alloc(cur_part);

                if(block_lba == -1){
                    printk("file_Write: block_bitmap_alloc for situation 3 failed.\n");
                    return -1;
                }
                all_blocks[block_idx++] = block_lba;

                block_bitmap_idx = block_lba  - cur_part->sb->data_start_lba;
                bitmap_sync(cur_part, block_bitmap_idx, BLOCK_BITMAP);
            }
            ide_write(cur_part->my_disk, indirect_block_table, all_blocks+12, 1);
        }
    }

    // 用到的地址已经收到all_blocks中,下面开始写数据
    bool first_write_block = true;
    file->fd_pos = file->fd_inode->i_size - 1;
    // 设置fd_pos为-1, 下面写数据时随时更新

    while(bytes_written < count) {
        memset(io_buf, 0, BLOCK_SIZE);
        sec_idx = file->fd_inode->i_size / BLOCK_SIZE;
        sec_lba = all_blocks[sec_idx];
        sec_off_bytes = file->fd_inode->i_size % BLOCK_SIZE;
        sec_left_bytes = BLOCK_SIZE - sec_off_bytes;

        // 判断此次写入硬盘的数据大小
        chunk_size = size_left < sec_left_bytes ? size_left : sec_left_bytes;

        if(first_write_block){
            ide_read(cur_part->my_disk, sec_lba, io_buf, 1);
            first_write_block = false;
        }

        memcpy(io_buf + sec_off_bytes, src, chunk_size);
        ide_write(cur_part->my_disk, sec_lba, io_buf, 1);
        printk("file_write at lba: 0x%x, size: %d\n", sec_lba, chunk_size);

        src += chunk_size;
        file->fd_inode->i_size += chunk_size;
        file->fd_pos  += chunk_size;
        bytes_written += chunk_size;
        size_left -= chunk_size;
    }
    inode_sync(cur_part, file->fd_inode, io_buf);
    sys_free(all_blocks);
    sys_free(io_buf);
    return bytes_written;
}

