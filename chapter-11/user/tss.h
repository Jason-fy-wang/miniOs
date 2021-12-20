#ifndef _USER_TSS_H_
#define _USER_TSS_H_
#include "thread.h"

#define PG_SIZE 4096

void tss_init(void);

void update_tss_esp(struct task_struct *pthread);

#endif // _USER_TSS_H_

