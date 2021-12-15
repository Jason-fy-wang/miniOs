[bits 32]
%define ERROR_CODE nop  ;若在相关的异常中CPU已经自动压如了错误码,为保持格式统一,这里不做操作
%define ZERO push 0     ; 若没有自动压入错误码,为了统一栈中格式,手工压入一个0

extern put_str;
section .data
intr_str db  "interrupt occur!", 0xa,0

global intr_entry_table

intr_entry_table:

%macro VECTOR 2
section .text
intr%1entry:
    %2
    push intr_str
    call put_str
    add esp,4

    ; 如果是从片上进入的中断,除了往从片上发送EOI 外, 还要往主片上发送EOI
    mov al, 0x20
    out 0x10, al        ; 向从片发送EOI
    out 0x20, al        ; 向主片发送EOI

    add esp, 4      ; 跳过error_code
    iret

section .data
    dd intr%1entry  ; 存储各个中断入口程序的地址,形成intr_entry_table 数组
%endmacro

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
VECTOR 0x0C, ERROR_CODE
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

VECTOR 0x20, ZERO