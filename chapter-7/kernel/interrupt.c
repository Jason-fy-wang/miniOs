#include "interrupt.h"
#include "global.h"
#include "io.h"
#include "print.h"

#define PIC_M_CTRL 0x20
#define PIC_M_DATA 0x21
#define PIC_S_CTRL 0xa0
#define PIC_S_DATA 0xa1

#define IDE_DESC_CNT 0x21  // 支持的中断数

// 中断门描述符结构体
struct gate_desc {
  uint16_t func_offset_low_word;
  uint16_t selector;
  uint8_t dcount;
  uint8_t attribute;
  uint16_t func_offset_high_word;
};

//静态函数声明
static void make_idt_desc(struct gate_desc *p_gdesc, uint8_t attr,
                          intr_handler function);
static struct gate_desc idt[IDE_DESC_CNT];           //中断门描述符数组
extern intr_handler intr_entry_table[IDE_DESC_CNT];  // 中断处理函数入口数组

static void make_idt_desc(struct gate_desc *p_gdesc, uint8_t attr,
                          intr_handler function) {
    p_gdesc->dcount = 0;
    p_gdesc->func_offset_low_word = (uint32_t)function & 0x0000ffff;
    p_gdesc->func_offset_high_word = (uint32_t)function & 0xffff0000;
    p_gdesc->attribute = attr;
    p_gdesc->selector = SELECTOR_K_CODE;
}

static void idt_desc_init(){
    int i=0;
    for(i=0; i< IDE_DESC_CNT; i++){
        make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
    }
    put_str("idt_desc_init done\n");
}

// 初始化 可编程中断控制器 8259A
static void pic_init(){
    // 初始化主片
    outb(PIC_M_CTRL, 0x11); //ICW1 级联, 打开IC4
    outb(PIC_M_DATA, 0x20); //ICW2 初始化中断向量号 0x20
    outb(PIC_M_DATA, 0x04); //ICW3  IR2 接从片
    outb(PIC_M_DATA, 0x01); //ICW4 8086处理器, 手动结束中断,非缓冲, 全嵌套
    // 初始化从片
    outb(PIC_S_CTRL, 0x11); //ICW1
    outb(PIC_S_DATA, 0x28); //ICW2  起始中断向量号为0x28
    outb(PIC_S_DATA, 0x02); //ICW3  连接主片的IR2
    outb(PIC_S_DATA, 0x01); //ICW4  同主

    // 打开主片的IR0, 也就是目前只接受时钟中断
    outb(PIC_M_DATA, 0xfe);  // OCW1 打开时钟中断
    outb(PIC_S_DATA, 0xFF); // 从OCW1 禁止所有中断

    put_str("pic init done\n");
}

// 有关中断的初始化
void idt_init(){
    put_str("idt_init start\n");
    idt_desc_init();        // 初始化中断描述符表
    pic_init();             // 初始化8259A

    // 加载idt
    uint64_t idt_operand = ((sizeof(idt)-1) | ((uint64_t)((uint32_t)idt << 16)));
    asm volatile("lidt %0;"::"m"(idt_operand));
    put_str("idt_init done\n");
}


