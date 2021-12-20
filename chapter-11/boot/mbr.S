;  mbr 主引导程序
%include "boot.inc"
SECTION MBR vstart=BIOSENTRY
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov sp, BIOSENTRY
    mov ax, 0xb800
    mov gs, ax

; 清屏
; 利用0x06 号功能, 上卷全部行, 则可清屏
; ----------------------------------------------------------------
;   INT 0x10   功能号:0x06  功能描述: 上卷窗口
; ----------------------------------------------------------------
; 输入:
; AH: 功能号: 0x06
; AL: 上卷的行数(如果为0, 则表示全部)
; BH = 上卷行的属性
; (CL,CH)= 窗口左下角(x,y)的位置
; (DL,DH)= 窗口右下角(x,y)的位置
; 无返回值
    mov ax, 0x0600
    mov bx, 0x0700
    mov cx, 0
    mov dx, 184fh ;右下角(80,25) 80*25即一行80个字符,25行. 0x18=24, 0x4f=80
    int 0x10

; 输入字符: 1 MBR
    mov byte [gs:0x00], '1'
    mov byte [gs:0x01], 0xA4
    mov byte [gs:0x02], ''
    mov byte [gs:0x03], 0xA4
    mov byte [gs:0x04], 'M'
    mov byte [gs:0x05], 0xA4
    mov byte [gs:0x06], 'B'
    mov byte [gs:0x07], 0xA4
    mov byte [gs:0x08], 'R'
    mov byte [gs:0x09], 0xA4

    mov eax, LOADER_START_SECTOR ; loader其实扇区LBA 地址
    mov bx, LOADER_BASE_ADDR    ; 要加载的位置
    mov cx, 4                   ; 要加载的扇区数量
    call rd_disk_m_16

    jmp LOADER_BASE_ADDR +0x300    ; 跳转到loader执行,即交接到下一棒

;-------------------------------
; 功能: 读取硬盘n个扇区
;-------------------------------
; eax = LBA 扇区号
; bx = 将要读入的内存位置
; cx = 读入的扇区数
rd_disk_m_16:
    mov esi, eax    ; backup eax
    mov di,cx       ; backup cx

; 读硬盘
; 第一步: 设置要读取的扇区数量
    mov dx, 0x1f2
    mov al, cl
    out dx,al

    mov eax, esi

; 第二步: 将LBA 地址存入 0x1F4 ~ 0x1f6
    ; LBA 7~0 位写入端口 0x1f3
    mov dx, 0x1f3
    out dx, al

    ; LBA 15~8 位 写入端口 0x1f4
    mov cl, 8
    shr eax,cl
    mov dx, 0x1f4
    out dx, al

    ;LBA 23~16位 写入端口 0x1f5
    shr eax,cl
    mov dx, 0x1f5
    out dx, al

    shr eax, cl
    and al, 0x0f        ;LBA 第24~27位
    or al, 0xE0         ; 设置0x1f6的7~4位为 1110, 表示LBA 模式
    mov dx, 0x1f6
    out dx, al


; 第三步: 向0x1f7 端口写入读命令 0x20
    mov dx, 0x1f7
    mov al, 0x20
    out dx, al


; 第四步: 检测硬盘状态
.not_ready:
    ; 0x1f7 写时表示命令,读时表示 状态
    nop
    in al, dx
    and al, 0x88

    cmp al,0x08
    jnz .not_ready

; 第五步: 从0x1f0 端口读取数据
    mov ax, di      ; di为要读取的扇区数量
    mov dx, 256
    mul dx          ; 每个扇区512字节,每次读取一个字,共需要 di*512/2 = di*256 次
    mov cx, ax
    mov dx, 0x1f0
.go_on_read:
    in ax, dx
    mov [bx], ax
    add bx, 2
    loop .go_on_read
    ret

    times 510-($-$$) db 0
    db 0x55, 0xAA



