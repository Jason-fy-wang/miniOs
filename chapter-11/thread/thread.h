#ifndef __KERNEL_THREAD_H__
#define __KERNEL_THREAD_H__
#include "stdint.h"
#include "list.h"
#include "memory.h"

typedef void thread_func(void*);

enum task_status {
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITTING,
    TASK_HANGING,
    TASK_DIED
};

/*********中断栈 intr_stack
 * 此结构用于中断发生时保护程序(线程或进程)的上下文环境:
 * 进程或线程被外部或软中断打断时,会按照此结构压入上下文寄存器
 * intr_exit中的出栈操作是此结构的逆操作.此栈在线程自己的内核栈中位置固定,所在页的最顶端
 */
struct intr_stack{
    uint32_t vec_no;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy; // 虽然pushad把esp也压入,但esp是动态变化的,所以会被popad忽略
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    // 由cpu从低特权级进入高特权级压入
    uint32_t err_code;  //
    void (*eip)(void);
    uint32_t cs;
    uint32_t eflags;
    void* esp;
    uint32_t ss;
};

/********* 线程栈: thread stack
 * 线程自己的栈,用于存储线程中待执行的函数
 * 此结构在线程自己的内核栈中位置不固定
 * 仅用在 switch_to 时保存线程环境
 * 实际位置 取决于实际运行情况
 */
struct thread_stack {
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;
    // 线程第一次执行时,eip用于指向待调用的函数kernel_thread,其他时候,eip是指向switch_to的返回地址
    void (*eip) (thread_func *func, void *func_arg);

    // 第一次被调度上cpu时使用
    // unused_ret只为占位置充数为返回地址
    void(*unused_retaddr);
    thread_func* function;  // kernel_thread所调用的函数名
    void *func_arg;         // kernel_thread所调用的函数所需参数
};

// 进程或线程的PCB, 程序控制块
struct task_struct{
    uint32_t* self_kstack;   //各内核线程都用自己的内核栈
    enum task_status status;
    uint8_t priority;
    char name[16];
    uint8_t ticks;          // 每次在处理器上执行的时间嘀嗒数
    uint32_t elapsed_ticks; // 占用了多少cpu嘀嗒数

    struct list_elem general_tag;

    struct list_elem all_list_tag;

    uint32_t* pgdir;        // 继承自己页表的虚拟地址

    struct virtual_addr userprog_vaddr; // 用户进程的虚拟地址

    uint32_t stack_magic; // 栈边界标记,用于检测栈溢出
};

extern struct list thread_ready_list;          // 就绪队列
extern struct list thread_all_list;            // 所有任务队列

void thread_create(struct task_struct *pthread, thread_func function, void *func_arg);
void init_thread(struct task_struct *pthread, char*name, int prio);

struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_arg);

void schedule(void);

struct task_struct* running_thread(void);

void thread_block(enum task_status stat);

void thread_unblock(struct task_struct *pthread);

void thread_init(void);

#endif /* __KERNEL_THREAD_H__ */

