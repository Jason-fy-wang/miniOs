[bits 32]
%define ERROR_CODE nop  ;若在相关的异常中CPU已经自动压如了错误码,为保持格式统一,这里不做操作
%define ZERO push 0     ; 若没有自动压入错误码,为了统一栈中格式,手工压入一个0

extern idt_table;

section .data
intr_str db  "interrupt occur!", 0xa,0

global intr_entry_table

intr_entry_table:

%macro VECTOR 2
section .text
intr%1entry:
    %2

    ;save context
    push ds
    push es
    push fs
    push gs
    pushad  ;入栈顺序: EAX, ECX, EDX, EBX,ESP,EBP,ESI,EDI

    ; 如果是从片上进入的中断,除了往从片上发送EOI 外, 还要往主片上发送EOI
    mov al, 0x20
    out 0xa0, al        ; 向从片发送EOI
    out 0x20, al        ; 向主片发送EOI

    push %1
    call [idt_table+%1*4]
    jmp intr_exit

section .data
    dd intr%1entry  ; 存储各个中断入口程序的地址,形成intr_entry_table 数组
%endmacro

section .text
global intr_exit
intr_exit:
    ; restore context
    add esp, 4
    popad
    pop gs
    pop fs
    pop es
    pop ds
    add esp, 4      ; skip error_code
    iretd

VECTOR 0x00, ZERO
VECTOR 0x01, ZERO
VECTOR 0x02, ZERO
VECTOR 0x03, ZERO
VECTOR 0x04, ZERO
VECTOR 0x05, ZERO
VECTOR 0x06, ZERO
VECTOR 0x07, ZERO
VECTOR 0x08, ERROR_CODE
VECTOR 0x09, ZERO
VECTOR 0x0A, ERROR_CODE
VECTOR 0x0B, ERROR_CODE
VECTOR 0x0C, ZERO
VECTOR 0x0D, ERROR_CODE
VECTOR 0x0E, ERROR_CODE
VECTOR 0x0F, ZERO
VECTOR 0x10, ZERO
VECTOR 0x11, ERROR_CODE
VECTOR 0x12, ZERO
VECTOR 0x13, ZERO

VECTOR 0x14, ZERO
VECTOR 0x15, ZERO
VECTOR 0x16, ZERO
VECTOR 0x17, ZERO
VECTOR 0x18, ERROR_CODE
VECTOR 0x19, ZERO
VECTOR 0x1a, ERROR_CODE
VECTOR 0x1b, ERROR_CODE
VECTOR 0x1c, ZERO
VECTOR 0x1d, ERROR_CODE
VECTOR 0x1e, ERROR_CODE
VECTOR 0x1f, ZERO

VECTOR 0x20, ZERO   ; 时钟中断入口
VECTOR 0x21, ZERO   ; 键盘中断对应的入口
VECTOR 0x22, ZERO   ; 级联
VECTOR 0x23, ZERO   ; 串口2对应入口
VECTOR 0x24, ZERO   ; 串口1对应入口
VECTOR 0x25, ZERO   ; 并口2对应入口
VECTOR 0x26, ZERO   ; 软盘对应入口
VECTOR 0x27, ZERO   ; 并口1对应入口
VECTOR 0x28, ZERO   ; 实时时钟对应入口
VECTOR 0x29, ZERO   ; 重定向
VECTOR 0x2a, ZERO   ; 保留
VECTOR 0x2b, ZERO   ; 保留
VECTOR 0x2c, ZERO   ; ps/2 鼠标
VECTOR 0x2d, ZERO   ; fpu浮点单元异常
VECTOR 0x2e, ZERO   ; 硬盘
VECTOR 0x2f, ZERO   ; 保留


;;;;;;;;;;; 0x80号中断;;;;;;;;;;;
[bits 32]
extern syscall_table
section .text

global syscall_handler
syscall_handler:
    ; 1.save context
    push 0              ; 压入0,使栈中格式统一
    push ds
    push es
    push fs
    push gs
    pushad          ; 入栈顺序: EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI

    push 0x80
    ;2.系统调用传入参数
    push edx        ; arg3
    push ecx        ; arg2
    push ebx        ; arg1
    ;3. 调用子功能函数
    call [syscall_table + eax * 4]
    add esp, 12

    ;4. 将call调用后的返回值存入当前内核栈中eax的位置
    mov [esp + 8 * 4], eax
    jmp intr_exit

