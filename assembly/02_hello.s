global _start
section .text
_start:
    mov     eax, 1  ; 1号系统调用
    mov     edi, 1  ;
    mov     esi, message
    mov     edx, 13
    syscall

    mov     eax, 60
    xor     edi, edi
    syscall

section .data

message:
    db    "Hello, world",10 ;10 是换行
