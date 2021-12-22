#ifndef _DEVICE_IDE_H_
#define _DEVICE_IDE_H_

#include "stdint.h"
#include "sync.h"
#include "global.h"
#include "bitmap.h"

// 分区结构
struct partition{
    uint32_t start_lba;     //起始扇区
    uint32_t  sec_cnt;      // 扇区数
    struct disk* my_disk;   // 分区所属的硬盘
    struct list_elem part_tag;  //用于队列中的标记
    char name[8];               // 分区名称
    struct super_block* sb;     //本分区的超级块
    struct bitmap block_bitmap; // 块位图
    struct bitmap inode_bitmap; // i节点位图
    struct list open_inodes;    // 本分区打开的i节点队列
};

// 硬盘结构
struct disk{
    char name[8];
    struct ide_channel* my_channel;
    uint8_t dev_no;
    struct partition prim_parts[4];
    struct partition logic_parts[8];
};

//ata通道结构
struct ide_channel{
    char name[8];       //本ata通道名称
    uint16_t port_base; //本通道使用的起始端口号
    uint8_t irq_no;     // 本通道所适用的终端号
    struct lock lock;   // 锁
    bool expecting_intr;    // 表示等待硬盘的中断
    struct semaphore disk_done;
    struct disk devices[2];     //一个通道上连接两个设备,一主一从
};

void intr_hd_handler(uint8_t irq_no);
void ide_init(void);
extern uint8_t channel_cnt;        //按硬盘数计算的通道数
extern struct ide_channel channels[];     //两个ide通道
extern struct list partition_list;     //分区队列
void ide_read(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt);
void ide_write(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt);

#endif // _DEVICE_IDE_H_

