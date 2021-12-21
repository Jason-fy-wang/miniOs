#ifndef _DEVICE_IO_QUEUE_H_
#define _DEVICE_IO_QUEUE_H_

#include "stdint.h"
#include "sync.h"

#define BUFSIZE 64

// 环形队列
struct ioqueue{
    struct lock lock;
    // 生产者,缓冲区不满时就继续往里面放数据.否则就睡眠.此项记录哪个生产者在此缓冲区上睡眠
    struct task_struct* producer;
    // 消费者,缓冲区不为空时就继续从里面拿数据.否则就是睡眠. 此项记录哪个消费者在此缓冲区上睡眠
    struct task_struct* consumer;

    char buffer[BUFSIZE];
    int32_t head;   // 队首
    int32_t tail;   // 队尾
};


void ioq_putchar(struct ioqueue* ioq, char byte);

char ioq_getchar(struct ioqueue* ioq);

bool ioq_full(struct ioqueue* ioq);

bool ioq_empty(struct ioqueue* ioq);

void ioqueue_init(struct ioqueue* ioq);

#endif // _DEVICE_IO_QUEUE_H_

