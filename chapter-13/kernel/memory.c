#include "memory.h"
#include "bitmap.h"
#include "stdint.h"
#include "global.h"
#include "debug.h"
#include "print.h"
#include "string.h"
#include "sync.h"
#include "interrupt.h"

/***************  位图地址 ********************
 * 因为0xc009f000是内核主线程栈顶，0xc009e000是内核主线程的pcb.
 * 一个页框大小的位图可表示128M内存, 位图位置安排在地址0xc009a000,
 * 这样本系统最大支持4个页框的位图,即512M */
#define MEM_BITMAP_BASE 0xc009a000
// 0xc0000000 是内核从虚拟地址 3G起,0x100000 指跨过低端内存1M
#define K_HEAP_START 0xc0100000

#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)
#define PTE_IDX(addr)  ((addr & 0x003ff000) >> 12)

// 内存池结构
struct pool {
    struct bitmap pool_bitmap;  // 本内存池用到的位图结构,用于管理物理内存
    uint32_t phy_addr_start;    //  本内存池所管理物理内存的起始地址
    uint32_t pool_size;         //  本内存池字节容量
    struct lock lock;
};

struct pool kernel_pool, user_pool; // 生成内核内存池和用户内存池
struct virtual_addr kernel_vaddr;   // 用来给内核分配虚拟地址

// 内存池
struct arena {
    struct mem_block_desc* desc;
    uint32_t cnt;
    bool large;             // large为true时,cnt表示的是页框数,否则cnt表示空闲mem_block数
};

struct mem_block_desc k_block_descs[DESC_CNT];  // 内核内存块描述符数组

// 在PF 表示的虚拟内存池中申请cnt个虚拟页,成功则返回虚拟页的起始地址,失败则返回NULL
static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt){
    int vaddr_start = 0,  bit_idx_start = -1;
    uint32_t cnt = 0;
    if(pf == PF_KERNEL){
        bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);
        if(bit_idx_start == -1){
            return NULL;
        }
        while(cnt < pg_cnt){
            bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start+cnt, 1);
            cnt++;
        }
        vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start* PG_SIZE;
    }else { // 用户申请虚拟内存
        struct task_struct *cur = running_thread();
        bit_idx_start = bitmap_scan(&cur->userprog_vaddr.vaddr_bitmap, pg_cnt);
        if(bit_idx_start == -1){
            return NULL;
        }
        while(cnt < pg_cnt){
            bitmap_set(&cur->userprog_vaddr.vaddr_bitmap, bit_idx_start+cnt, 1);
            cnt++;
        }
        vaddr_start = cur->userprog_vaddr.vaddr_start + bit_idx_start*PG_SIZE;
        // 0xc0000000-PG_SIZE 作为用户3级栈,在start_process被分配
        ASSERT((uint32_t)vaddr_start < (0xc0000000 - PG_SIZE));
    }
    return (void*) vaddr_start;
}

//得到虚拟地址vaddr对应的pte 指针
uint32_t* pte_ptr(uint32_t vaddr){
    uint32_t *pte = (uint32_t *)(0xffc00000 + ((vaddr & 0xffc00000) >> 10)+PTE_IDX(vaddr)*4);
    return pte;
}

//得到虚拟地址对应的pde指针
uint32_t* pde_ptr(uint32_t vaddr){
    uint32_t* pde = (uint32_t*)((0xfffff000) + PDE_IDX(vaddr)*4);
    return pde;
}

// 在m_pool指向的物理内存中分配1个物理页,成功则返回物理页的物理地址,失败返回NULL
static void* palloc(struct pool* m_pool){
    // 扫描或设置位图要保证原子性
    int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1); //找一个物理页
    if(bit_idx == -1){
        return NULL;
    }

    bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);
    uint32_t page_phyaddr = ((bit_idx * PG_SIZE) + m_pool->phy_addr_start);
    return (void*)page_phyaddr;
}

// 将物理地址 pg_phy_addr 回收到物理内存池中
void pfree(uint32_t pg_phy_addr){
    struct pool* mem_pool;
    uint32_t bit_idx  = 0;
    if(pg_phy_addr >= user_pool.phy_addr_start){    // 用户物理内存池
        mem_pool = &user_pool;
        bit_idx = (pg_phy_addr - user_pool.phy_addr_start) / PG_SIZE;
    }else {     // 内核物理内存池
        mem_pool = &kernel_pool;
        bit_idx = (pg_phy_addr - kernel_pool.phy_addr_start) / PG_SIZE;
    }
    bitmap_set(&mem_pool->pool_bitmap, bit_idx, 0);
}

// 页表中添加虚拟地址vaddr与物理地址page_phyaddr的映射
static void page_table_add(void* _vaddr, void* _page_phyaddr){
    uint32_t vaddr = (uint32_t)_vaddr;
    uint32_t page_phyaddr = (uint32_t)_page_phyaddr;
    uint32_t *pde = pde_ptr(vaddr);
    uint32_t *pte = pte_ptr(vaddr);
    // 判断P位,确保PDE已经创建, 否则会引发page_fault
    if((*pde) & 0x00000001){
        // 判断页表项 有没有存在
        ASSERT(!((*pte) & 0x00000001));
        if(!((*pte) & 0x00000001)){
            *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        }else { // 页表项 不存在
            PANIC("pte repeat");
        }
    }else { // 页目录项不存在
        // 分配页目录项
        uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool);
        *pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);

        memset((void*)((int)pte&0xfffff000),0, PG_SIZE);

        ASSERT(!((*pte)&0x00000001));
        *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
    }
}

// 去掉页表中虚拟地址 vaddr的映射, 只去掉vaddr对应的pte
static void page_table_pte_remove(uint32_t vaddr){
    uint32_t* pte = pte_ptr(vaddr);
    *pte &= ~PG_P_1;        // 将页表项pte的P位 置为0
    asm volatile("invlpg %0": :"m"(vaddr):"memory");    // 更新tlb
}

// 在虚拟地址池释放以 vaddr 起始的连续 pg_cnt 个虚拟页地址
static void vaddr_remove(enum pool_flags pf, void* _vaddr, uint32_t pg_cnt){
    uint32_t bit_idx_start = 0;
    uint32_t vaddr = (uint32_t)_vaddr, cnt=0;
    if(pf == PF_KERNEL){        // 内核虚拟内存池
        bit_idx_start = (vaddr - kernel_vaddr.vaddr_start) / PG_SIZE;
        while(cnt < pg_cnt){
            bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 0);
        }
    }else { //  用户虚拟内存池
        struct task_struct* cur_thread = running_thread();
        bit_idx_start = (vaddr - cur_thread->userprog_vaddr.vaddr_start) / PG_SIZE;
        while(cnt < pg_cnt){
            bitmap_set(&cur_thread->userprog_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 0);
        }
    }
}


// 分配pg_cnt个页空间，成功则返回起始虚拟地址，失败则返回NULL
void * malloc_page(enum pool_flags pf, uint32_t pg_cnt){
    ASSERT(pg_cnt >0 && pg_cnt < 3840);      // 目前内存小,3840 表示不可超过15M
    /******
     * 分配动作:
     * 1.通过 vaddr_get 在虚拟内存池中申请虚拟地址
     * 2.通过palloc在物理内存池中申请 物理页
     * 3.通过page_table_add 将得到的虚拟地址和物理地址在页表中完成映射
     */
    void* vaddr_start = vaddr_get(pf, pg_cnt);
    if(vaddr_start == NULL){
        return NULL;
    }

    uint32_t vaddr =(uint32_t)vaddr_start;
    uint32_t cnt = pg_cnt;

    struct pool *mem_pool = pf & PF_KERNEL? & kernel_pool: &user_pool;

    while(cnt-- >0){
        void *page_phyaddr = palloc(mem_pool);
        if(page_phyaddr == NULL){
            // 失效时,需要将曾经申请的虚拟地址和物理页全部回滚, 在将来完成.
            return NULL;
        }
        page_table_add((void*)vaddr, page_phyaddr);
        vaddr += PG_SIZE;       // 下一个虚拟页
    }
    return vaddr_start;

}

// 释放以虚拟地址vaddr为起始的cnt个物理页框
void mfree_page(enum pool_flags pf, void* _vaddr, uint32_t pg_cnt){
    uint32_t pg_phy_addr;
    uint32_t vaddr = (uint32_t)_vaddr;
    uint32_t page_cnt = 0;

    ASSERT(pg_cnt >= 1 && vaddr % PG_SIZE == 0);
    //获取虚拟地址vaddr对应的物理地址
    pg_phy_addr = addr_v2p(vaddr);

    //确保待释放的物理内存在 低端 1M+1KB大小的页目录 + 1KB大小的页表地址方位外
    // 因为此 空间为 页表在内存中的位置
    ASSERT((pg_phy_addr % PG_SIZE) == 0 && pg_phy_addr >= 0x102000);

    // 判断 pg_phy_addr属于用户物理内存池还是 内核物理内存池
    if(pg_phy_addr >= user_pool.phy_addr_start){        // 用户内存池
        vaddr -= PG_SIZE;
        while(page_cnt < pg_cnt){
            vaddr += PG_SIZE;
            pg_phy_addr = addr_v2p(vaddr);

            //确保物理地址位于用户物理内存池
            ASSERT((pg_phy_addr % PG_SIZE)==0 && pg_phy_addr >= user_pool.phy_addr_start);
            //先将对应的物理页归还到 内存池
            pfree(pg_phy_addr);
            //再从页表中清除此虚拟地址所在的页表项 pte
            page_table_pte_remove(vaddr);
            page_cnt++;
        }
        // 清空虚拟地址的位图中的相应位
        vaddr_remove(pf, _vaddr, pg_cnt);
    }else {     // 内核物理内存池
        vaddr -= PG_SIZE;
        while(page_cnt < pg_cnt){
            vaddr += PG_SIZE;
            pg_phy_addr = addr_v2p(vaddr);
            ASSERT((pg_phy_addr % PG_SIZE) == 0 && pg_phy_addr >= kernel_pool.phy_addr_start && pg_phy_addr < user_pool.phy_addr_start);
            pfree(pg_phy_addr);

            page_table_pte_remove(vaddr);
            page_cnt++;
        }
        vaddr_remove(pf, _vaddr, pg_cnt);
    }
}

// 从内核物理内存池中申请1个页内存, 成功则返回其虚拟地址,失败则返回NULL
void* get_kernel_pages(uint32_t page_cnt){
    lock_acquire(&kernel_pool.lock);
    void* vaddr = malloc_page(PF_KERNEL, page_cnt);
    if(vaddr !=NULL){
        memset(vaddr, 0, page_cnt * PG_SIZE);
    }
    lock_release(&kernel_pool.lock);
    return vaddr;
}

// 在用户空间中申请内存
void* get_user_pages(uint32_t pg_cnt){
    lock_acquire(&user_pool.lock);
    void *vaddr = malloc_page(PF_USER, pg_cnt);
    memset(vaddr, 0, pg_cnt * PG_SIZE);
    lock_release(&user_pool.lock);
    return vaddr;
}

// 将地址vaddr 与pf池中的物理地址管理,仅支持一页空间分配
void* get_a_page(enum pool_flags pf, uint32_t vaddr){
    struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool: &user_pool;
    lock_acquire(&mem_pool->lock);
    struct task_struct* cur = running_thread();
    int32_t bit_idx = -1;

    // 若当前是用户进程申请用户内存, 就用户进程自己的虚拟地址位图
    if(cur->pgdir != NULL && pf == PF_USER){
        bit_idx = (vaddr - cur->userprog_vaddr.vaddr_start) / PG_SIZE;
        ASSERT(bit_idx > 0);
        bitmap_set(&cur->userprog_vaddr.vaddr_bitmap, bit_idx, 1);
    }else if(cur->pgdir == NULL && pf == PF_KERNEL){
    // 如果是内核线程申请内核内存,就修改kernel_vaddr
        bit_idx = (vaddr - kernel_vaddr.vaddr_start) / PG_SIZE;
        ASSERT(bit_idx > 0);
        bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx, 1);
    }else {     // 申请内存失败
        PANIC("get_a_page: not allow kernel alloc userspace or user alloc kernelspace by get_a_page");
    }
    void* page_phyaddr = palloc(mem_pool);
    if(page_phyaddr == NULL){
        return NULL;
    }
    page_table_add((void*)vaddr, page_phyaddr);
    lock_release(&mem_pool->lock);
    return (void*)vaddr;
}

// 得到虚拟地址 映射到的 物理地址
uint32_t addr_v2p(uint32_t vaddr){
    uint32_t* pte = pte_ptr(vaddr);

    // *pte 是页表所在的物理页框地址,去掉其低12位的页表项属性 + 虚拟地址vaddr的低12位
    return ((*pte) & 0xfffff000) + (vaddr & 0x00000fff);
}

// 初始化内存池
static void mem_pool_init(uint32_t all_mem){
    put_str(" mem_pool_init start\n");
    uint32_t page_table_size = PG_SIZE * 256;

    uint32_t used_mem = page_table_size + 0x100000; // 0x100000为低端1M 内存
    uint32_t free_mem = all_mem - used_mem;
    uint16_t all_free_pages = free_mem / PG_SIZE;

    uint16_t kernel_free_page = all_free_pages/2;
    uint16_t user_free_pages = all_free_pages - kernel_free_page;

    // 简化位图操作,余数不做处理,坏处这样会丢内存. 好处是 不用做内存越界检查, 因为位图表示的内存少于实例物理内存
    uint32_t kbm_length = kernel_free_page/8;   //kbm: kernel bitmap

    uint32_t ubm_length = user_free_pages / 8; //ubm: user bitmap

    uint32_t kp_start = used_mem;   // kernel pool start

    uint32_t up_start = kp_start + kernel_free_page*PG_SIZE;  // user pool start

    kernel_pool.phy_addr_start = kp_start;
    user_pool.phy_addr_start = up_start;

    kernel_pool.pool_size = kernel_free_page * PG_SIZE;
    user_pool.pool_size = user_free_pages * PG_SIZE;

    kernel_pool.pool_bitmap.btmp_bytes_len = kbm_length;
    user_pool.pool_bitmap.btmp_bytes_len = ubm_length;

    // 内核内存池的位图先定位得MEM_BITMAP_BASE 处
    kernel_pool.pool_bitmap.bits = (void*)MEM_BITMAP_BASE;

    user_pool.pool_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length);

    put_str("  kernel_pool_bitmap start:");
    put_int((int)kernel_pool.pool_bitmap.bits);
    put_str("  kernel_pool_phy_addr_start: ");
    put_int(kernel_pool.phy_addr_start);
    put_str("\n");
    put_str("user_pool_bitmap_start: ");
    put_int((int)user_pool.pool_bitmap.bits);
    put_str("   user_pool_phy_addr_Start: ");
    put_int(user_pool.phy_addr_start);

    // 位图置0
    bitmap_init(&kernel_pool.pool_bitmap);
    bitmap_init(&user_pool.pool_bitmap);

    lock_init(&kernel_pool.lock);
    lock_init(&user_pool.lock);

    // 初始化内核虚拟地址位图
    kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;
    // 暂时将虚拟地址位图定位在 内核内存池和用户内存池之外
    kernel_vaddr.vaddr_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length + ubm_length);

    kernel_vaddr.vaddr_start = K_HEAP_START;
    bitmap_init(&kernel_vaddr.vaddr_bitmap);

    put_str("  mem_pool_init done\n");
}

// 为malloc做准备
void block_desc_init(struct mem_block_desc* desc_array){
    uint16_t desc_idx, block_size=16;

    // 初始化每个,mem_block_desc描述符
    for(desc_idx=0; desc_idx<DESC_CNT;desc_idx++){
        desc_array[desc_idx].block_size = block_size;
        desc_array[desc_idx].blocks_per_arena = (PG_SIZE - sizeof(struct arena)) / block_size;
        list_init(&desc_array[desc_idx].free_list);
        block_size *= 2;
    }
}
// 返回arena中第idx个内存块地址
static struct mem_block* arena2block(struct arena* a, uint32_t idx){
    return (struct mem_block*)((uint32_t )a + sizeof(struct arena) + idx * a->desc->block_size);
}

// 返回内存块b所在的arena地址
static struct arena* block2arena(struct mem_block* b){
    return (struct arena*) ((uint32_t)b & 0xfffff000);
}

// 在堆中申请size个字节内存
void* sys_malloc(uint32_t size){
    enum pool_flags pf;
    struct pool* mem_pool;
    uint32_t pool_size;
    struct mem_block_desc* descs;
    struct task_struct* cur_thread = running_thread();

    //判断使用哪个内存池
    if(cur_thread->pgdir == NULL){  //内核线程
        pf = PF_KERNEL;
        pool_size = kernel_pool.pool_size;
        mem_pool = &kernel_pool;
        descs = k_block_descs;
    }else { //用户进程
        pf = PF_USER;
        pool_size = user_pool.pool_size;
        mem_pool = &user_pool;
        descs = cur_thread->u_block_desc;
    }

    // 申请内存不在内存池容量范围内,则直接返回NULL
    if(!(size > 0 && size < pool_size)){
        return NULL;
    }
    struct arena* a;
    struct mem_block* b;
    lock_acquire(&mem_pool->lock);
    //超过最大内存块1024,就分配页
    if(size > 1024){
        uint32_t page_cnt = DIV_ROUND_UP(size + sizeof(struct arena), PG_SIZE);

        a = malloc_page(pf, page_cnt);
        if(a != NULL){
            memset(a, 0, page_cnt * PG_SIZE);
            // 对于分配的大块页框,将desc设置为NULL,cnt为页框数,large为true
            a->desc = NULL;
            a->cnt = page_cnt;
            a->large = true;
            lock_release(&mem_pool->lock);
            return (void*)(a+1);    // 跨过arena大小,把剩下内存返回
        }else { // 申请失败
            lock_release(&mem_pool->lock);
            return NULL;
        }
    }else { //申请内存小于1024,在各种规格的mem_block_desc中适配

        uint8_t desc_idx;

        // 从内存块描述符中匹配合适的内存块规格
        for(desc_idx = 0; desc_idx < DESC_CNT; desc_idx++){
            if(size <= descs[desc_idx].block_size){
                break;
            }
        }
        //若mem_block_desc的free_list中已经没有可用的mem_block,就创建新的arena提供mem_block
        if(list_empty(&descs[desc_idx].free_list)){
            a = malloc_page(pf,1);
            if(a == NULL){
                lock_release(&mem_pool->lock);
                return NULL;
            }
            memset(a,0, PG_SIZE);
            //对于分配的小块内存,将desc设置为相应内存块描述符,cnt设置为此arena可用的内存块数,large设置为false
            a->desc = &descs[desc_idx];
            a->large = false;
            a->cnt = descs[desc_idx].blocks_per_arena;
            uint32_t block_idx;
            enum intr_status old_status = intr_disable();
            //开始将arena拆分为内存块,并添加到内存块描述符的free_list中
            for(block_idx=0; block_idx<descs[desc_idx].blocks_per_arena;block_idx++){
                b = arena2block(a, block_idx);
                ASSERT(!elem_find(&a->desc->free_list, &b->free_elem));
                list_append(&a->desc->free_list, &b->free_elem);
            }
            intr_set_status(old_status);
        }
        //开始分配内存块
        b = elem2entry(struct mem_block, free_elem, list_pop(&descs[desc_idx].free_list));
        memset(b, 0, descs[desc_idx].block_size);

        a = block2arena(b);     // 获取内存块b所在的arena
        a->cnt--;
        lock_release(&mem_pool->lock);
        return (void*)b;
    }
}

// 回收内存 ptr
void sys_free(void* ptr){
    ASSERT(ptr != NULL);
    if(ptr != NULL){
        enum pool_flags pf;
        struct pool* mem_pool;

        //判断是线程还是进程
        if(running_thread()->pgdir == NULL){        //
            ASSERT((uint32_t)ptr >= K_HEAP_START);
            pf = PF_KERNEL;
            mem_pool = &kernel_pool;
        }else {         //

            pf = PF_USER;
            mem_pool = &user_pool;
        }
        lock_acquire(&mem_pool->lock);
        struct mem_block* b= ptr;
        struct arena* a = block2arena(b);   // 把block转换为arena,获取元信息

        ASSERT(a->large == false || a->large == true);

        if(a->desc == NULL && a->large == true) {           // 大于1024
            mfree_page(pf,a, a->cnt);
        }else {
            // 先将内存块回收到 free_list
            list_append(&a->desc->free_list, &b->free_elem);

            // 再判断此arena中的内存块是否都是空闲,如果是,就释放此arena
            if(++a->cnt == a->desc->blocks_per_arena){
                uint32_t block_idx;
                for(block_idx =0; block_idx < a->desc->blocks_per_arena; block_idx++){
                    struct mem_block* b = arena2block(a, block_idx);
                    ASSERT(elem_find(&a->desc->free_list, &b->free_elem));
                    list_remove(&b->free_elem);
                }
                mfree_page(pf,a, 1);
            }
        }
        lock_release(&mem_pool->lock);
    }
}


void mem_init(void){
    put_str(" mem_init start\n");
    uint32_t mem_bytes_total = (*(uint32_t*)(0x0b00));
    mem_pool_init(mem_bytes_total);
    block_desc_init(k_block_descs);
    put_str("mem_init_done \n");
}
