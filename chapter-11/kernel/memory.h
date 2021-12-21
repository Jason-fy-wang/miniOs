#ifndef __KERNEL_MEMORY_H__
#define __KERNEL_MEMORY_H__

#include "stdint.h"
#include "bitmap.h"

enum pool_flags{
    PF_KERNEL=1,        // 内核内存池
    PF_USER = 2         // 用户内存池
};
#define PG_P_1  1       // 页表项或页目录项存在属性位
#define PG_P_0  0       // 不存在属性位
#define PG_RW_R 0       // R/W 属性位 读/执行
#define PG_RW_W 2       // RW 属性位,  读/写/执行
#define PG_US_S 0       // U/S 属性, 系统级
#define PG_US_U 4       // U/S属性, 用户级

struct virtual_addr{
    struct bitmap vaddr_bitmap; // 虚拟地址用到的位图结构
    uint32_t vaddr_start;       // 虚拟地址起始地址
};

extern struct pool kernel_pool, user_pool;

uint32_t* pte_ptr(uint32_t vaddr);
uint32_t * pde_ptr(uint32_t vaddr);
void* malloc_page(enum pool_flags pf, uint32_t pg_cnt);
void* get_kernel_pages(uint32_t page_cnt);
void* get_user_pages(uint32_t pg_cnt);
void* get_a_page(enum pool_flags pf, uint32_t vaddr);
uint32_t addr_v2p(uint32_t vaddr);
void mem_init(void);

#endif /* __KERNEL_MEM*/

