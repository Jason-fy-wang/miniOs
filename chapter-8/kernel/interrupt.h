#ifndef __KERNEL_INTERRUPT_H__
#define __KERNEL_INTERRUPT_H__

typedef void* intr_handler;
void idt_init();

//  中断状态:INTR_OFF 值为 0, 关中断
// INTR_ON 值为 1, 表示开中断

enum intr_status{
    INTR_OFF,
    INTR_ON
};

enum intr_status intr_get_status();
enum intr_status intr_set_status(enum intr_status status);
enum intr_status intr_enable();
enum intr_status intr_disable();
#endif /* __KERNEL_INTERRUPT_H__ */
