; simple example demonstrating basic program format and layout

SECTION .data

; define constants
EXIST_SUCCESS equ 1
SYS_EXIT      equ 60   ;  call code for terminate

; Byte (b-bit) vaiable declarations
bVar1:  db  17
bVar2:  db  9
bResult: db 0

; word(16-bit) variable declarations

wVar1: dw  17000
wVar2: dw  9000
wResult: dw 0

; Double-word (32-bit) variable declarations
dVar1:  dd  17000000
dVar2:  dd  9000000
dResult: dd 0

; quadword(64-bit) variable declarations
qVar1:   dq    170000000
qVar2:   dq    90000000
qResult: dq    0

global _start
SECTION  .text
_start:
; perform  base addition operations
; Byte example

    mov     al, [bVar1]
    add     al, [bVar2]
    mov     [bResult], al

last:
    mov     eax, 60
    mov     edi, EXIST_SUCCESS
    ;xor    edi, edi
    syscall



