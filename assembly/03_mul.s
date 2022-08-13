; simple program to compute sum of 1 + 2^2 + ⋯ + 10^2 = 385

SECTION .data
SUSSESS equ 0
SYS_EXIT equ 60
n:  dd 10
sum:  dq 0



SECTION .text
global _start
_start:
; for (i =1; i<n; i++){
;   sum += i^2
;}

    mov  rbx, 1     ;
    mov  ecx, [n]   ; n
sumLoop:
    mov  rax, rbx   ; get i
    mul  rax        ; i^2
    add  [sum], rax
    inc rbx
    loop sumLoop

last:
    mov rax, SYS_EXIT
    mov rdi,  SUSSESS
    syscall
