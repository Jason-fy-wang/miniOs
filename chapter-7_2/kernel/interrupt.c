#include "interrupt.h"
#include "global.h"
#include "io.h"
#include "print.h"

#define PIC_M_CTRL 0x20
#define PIC_M_DATA 0x21
#define PIC_S_CTRL 0xa0
#define PIC_S_DATA 0xa1

#define IDE_DESC_CNT 0x21  // 支持的中断数

char * intr_name[IDE_DESC_CNT];
intr_handler idt_table[IDE_DESC_CNT];
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

static void general_intr_handler(uint8_t vec_nr){
    if(vec_nr == 0x27 || vec_nr == 0x2f){
        // IRQ7 IRQ15 会产生伪中断,无需处理
        return;
    }
    put_str("int vector: 0x");
    put_int(vec_nr);
    put_char('\n');
}

static void exception_init(void){
    int i;
    for(i = 0; i< IDE_DESC_CNT; i++){
        idt_table[i] = general_intr_handler;
        intr_name[i] = "unknown";
    }

    intr_name[0] = "#DE Divide Error";
    intr_name[1] = "#DB Debug Exception";
    intr_name[2] = "NMI Interrupt";
    intr_name[3] = "#BP Breakpoint Exception";
    intr_name[4] = "#OF Overflow Exception";
    intr_name[5] = "#BR Bound range Exceeded Exception";
    intr_name[6] = "#UD Invalid Opcode Exception";
    intr_name[7] = "#NM Device not Available Exception";
    intr_name[8] = "#DF Double Fault Exception";
    intr_name[9] = "Coprocessor Segment Exception";
    intr_name[10] = "#TS Invalid TSS Exception";
    intr_name[11] = "#NP Segment Not Present";
    intr_name[12] = "#SS Stack Fault Exception";
    intr_name[13] = "#GP General Protection Exception";
    intr_name[14] = "#PF Page-Fault Exception";
    //intr_name[15] = "";
    intr_name[16] = "#MF 0x87 FPU Floating-point Error";
    intr_name[17] = "#AC Alignment Check Exception";
    intr_name[18] = "#MC Machine-check Exception";
    intr_name[19] = "#XF SIMD Floating-point Exception";
}

// 有关中断的初始化
void idt_init(){
    put_str("idt_init start\n");
    idt_desc_init();        // 初始化中断描述符表
    exception_init();
    pic_init();             // 初始化8259A

    // 加载idt
    uint64_t idt_operand = ((sizeof(idt)-1) | ((uint64_t)((uint32_t)idt << 16)));
    asm volatile("lidt %0;"::"m"(idt_operand));
    put_str("idt_init done\n");
}


