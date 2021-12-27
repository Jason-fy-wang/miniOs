#ifndef _USER_PROCESS_H_
#define _USER_PROCESS_H_
#include "thread.h"
#include "stdint.h"
#define USER_STACK3_VADDR  (0xc0000000 - 0x1000)
#define USER_VADDR_START 0x8048000
#define default_prio 31

void process_execute(void* filename, char* name);

void start_process(void* filename_);
void process_activate(struct task_struct *pthread);
void page_dir_activate(struct task_struct *pthread);
uint32_t* create_page_dir(void);
void create_user_vaddr_bitmap(struct task_struct* user_prog);
#endif // _USER_PROCESS_H_

