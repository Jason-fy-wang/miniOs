#include "sync.h"
#include "debug.h"
#include "interrupt.h"

void sema_init(struct semaphore* psem, uint8_t value){
    psem->value = value;
    list_init(&psem->waiters);
}


void lock_init(struct lock* plock){
    plock->holder = NULL;
    plock->holder_repeat_nr = 0;
    sema_init(&plock->semaphore, 1);
}

void sema_down(struct semaphore* psema){
    enum intr_status old_status = intr_disable();
    while(psema->value  == 0){
        // 并且当前线程不在信号量 等待队列中
        ASSERT(!(elem_find(&psema->waiters, &running_thread()->generate_tag)));
        if(elem_find(&psema->waiters,&running_thread()->generate_tag)){
            PANIC("sema down: thread blocked has been in waiters_list\n");
        }
        // 信号量的值等于0,则表明不可用,则把自己添加到 等待队列中
        list_append(&psema->waiters, &running_thread()->generate_tag);
        thread_block(TASK_BLOCKED);
    }
    // 若value为1 或被唤醒后,会执行下面的代码,也就是获得了锁
    psema->value--;
    ASSERT(psema->value == 0);

    intr_set_status(old_status);
}

void sema_up(struct semaphore* psema){
    enum intr_status old_status  = intr_disable();
    ASSERT(psema->value  == 0);

    if(!(list_empty(&psema->waiters))){
        struct task_struct* thread_blocked = elem2entry(struct task_struct, generate_tag,list_pop(&psema->waiters));
        thread_unblock(thread_blocked);
    }

    psema->value++;
    ASSERT(psema->value  == 1);

    intr_set_status(old_status);
}

void lock_acquire(struct lock* plock){
    // 排除曾经已经持有锁 但还未将其释放的情况
    if(plock->holder != running_thread()){
        sema_down(&plock->semaphore);   // 
        plock->holder = running_thread();
        ASSERT(plock->holder_repeat_nr == 0);
        plock->holder_repeat_nr = 1;
    }else {
        plock->holder_repeat_nr++;
    }
}


void lock_release(struct lock* plock){
    ASSERT(plock->holder == running_thread());

    if(plock->holder_repeat_nr > 1){
        plock->holder_repeat_nr--;
        return;
    }
    ASSERT(plock->holder_repeat_nr == 1);
    plock->holder= NULL;
    plock->holder_repeat_nr = 0;
    sema_up(&plock->semaphore);
}


