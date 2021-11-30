; 主引导程序
; LOADER_BASE_ADDR equ 0xA000
; LOADER_START_SECTOR  equ 0x2

SECTION MBR vstart=0x7c00
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov sp, 0x7c00
    mov ax, 0xb800  ; 0xb800 显卡文本内存基址
    mov gs, ax
; 清屏
; 利用 0x06 号功能， 上卷全部行, 则可清屏
;--------------------------------------------------------
; INT 0X10  功能号 0x60  功能描述: 上卷窗口
;--------------------------------------------------------
; 输入:
; AH : 功能号 = 0x06
; AL = 上卷的行数 (如果为0,则表示全部)
; BH = 上卷行的属性
; (CL,CH) = 窗口左下角(x,y)的位置
; (DL,DH) = 窗口右下角(x,y)的位置
; 无返回值
    mov ax,0600H
    mov bx,0700H
    mov cx, 0       ;左上角 (0,0)
    mov dx, 184fh    ;右下角(80,25) 80*25即一行80个字符,25行. 0x18=24, 0x4f=80

    int 10h

; 输出背景色绿色, 前景色红色,并且跳动的字符串'1 MBR'

    mov byte [gs:0x00], '1'
    mov byte [gs:0x01], 0xA4

    mov byte [gs:0x02], ''
    mov byte [gs:0x03], 0xA4

    mov byte [gs:0x04], 'M'
    mov byte [gs:0x05], 0xAC

    mov byte [gs:0x06], 'B'
    mov byte [gs:0x07], 0xA4

    mov byte [gs:0x08], 'R'
    mov byte [gs:0x09], 0xAC

    jmp $

    times 510-($-$$) db 0
    db 0x55,0xAA