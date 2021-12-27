#ifndef _KERNEL_SYNC_H_
#define _KERNEL_SYNC_H_

#include "stdint.h"
#include "thread.h"
#include "list.h"

// 信号量
struct semaphore{
    uint8_t value;
    struct list waiters;
};

// 锁结构
struct lock{
    struct task_struct* holder;     // 锁的持有者
    struct semaphore semaphore;         // 用二元信号量实现锁
    uint32_t holder_repeat_nr;      // 锁的持有者重复申请锁的次数
};

void sema_init(struct semaphore* psem, uint8_t value);
void lock_init(struct lock* plock);
void sema_down(struct semaphore* psema);
void sema_up(struct semaphore* psema);
void lock_acquire(struct lock* plock);
void lock_release(struct lock* plock);

#endif // _KERNEL_SYNC_H_
