#include "ioqueue.h"
#include "global.h"
#include "debug.h"
#include "interrupt.h"
#include "thread.h"

// 初始化ioq
void ioqueue_init(struct ioqueue* ioq){
    lock_init(&ioq->lock);
    ioq->consumer = NULL;
    ioq->producer = NULL;
    ioq->head = 0;
    ioq->tail = 0;
}


static int32_t next_pos(int32_t pos){
    return (pos+1) % BUFSIZE;
}


bool ioq_full(struct ioqueue* ioq){
    ASSERT(intr_get_status() == INTR_OFF);
    return next_pos(ioq->head) == ioq->tail;
}

bool ioq_empty(struct ioqueue* ioq){
    ASSERT(intr_get_status() == INTR_OFF);
    return ioq->head == ioq->tail;
}
// 使当前消费者或生产者在此缓冲区上等待
static void ioq_wait(struct task_struct** waiter){
    ASSERT(*waiter == NULL &&  waiter != NULL);
    *waiter = running_thread();
    thread_block(TASK_BLOCKED);
}

static void wakeup(struct task_struct** waiter){
    ASSERT(*waiter != NULL);
    thread_unblock(*waiter);
    *waiter = NULL;
}

char ioq_getchar(struct ioqueue* ioq){
    ASSERT(intr_get_status() == INTR_OFF);
    //若缓冲区为空,把消费者ioq->consumer记为当前线程自己
    // 目的是将来生产者往缓冲区里面放东西后, 生产者知道唤醒哪个消费者
    while(ioq_empty(ioq)){
        lock_acquire(&ioq->lock);
        ioq_wait(&ioq->consumer);
        lock_release(&ioq->lock);
    }

    char byte = ioq->buffer[ioq->tail];
    ioq->tail = next_pos(ioq->tail);

    if(ioq->producer != NULL){
        wakeup(&ioq->producer);
    }

    return byte;
}


void ioq_putchar(struct ioqueue* ioq, char byte){
    ASSERT(intr_get_status() == INTR_OFF);

    // 如缓冲区已经满了,把生产者ioq->producer记为自己
    // 为的是当缓冲区的东西呗消费者 取完后, 让消费者知道唤醒哪个生产者
    while(ioq_full(ioq)){
        lock_acquire(&ioq->lock);
        ioq_wait(&ioq->producer);
        lock_release(&ioq->lock);
    }
    ioq->buffer[ioq->head] = byte;
    ioq->head = next_pos(ioq->head);

    if(ioq->consumer != NULL){
        wakeup(&ioq->consumer);
    }

}

