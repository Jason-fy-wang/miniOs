; mbr 主引导程序
%include "boot.inc"
SECTION MBR vstart=0x7c00
    mov ax,cs
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov fs,ax
    mov sp,0x7c00
    mov ax,0xb800
    mov gs,ax

;清屏
;利用0x06 号功能,上卷全部行,则可清屏
; ----------------------------------------------------------------
; INT 0x10  功能号: 0x06    功能描述: 上卷窗口
; ----------------------------------------------------------------
; 输入:
; AH 功能号 = 0x06
; AL = 上卷的行数 (如果为0,则表示全部)
; BH = 上卷行的属性
; (CL,CH) = 窗口左上角(x,y)位置
; (DL,DH) = 窗口右下角(x,y)位置
; 无返回值
    mov ax, 0x0600
    mov bx, 0x0700
    mov cx,0
    mov dx, 184fh

    int 10h
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

    mov eax, LOADER_START_SECTOR  ;起始扇区LBA地址
    mov bx, LOADER_BASE_ADDR    ; 要加载的地址
    mov cx, 1                 ; 待读入的扇区数
    call rd_disk_m_16

    jmp LOADER_BASE_ADDR
; ---------------------------------
; 功能:  读取硬盘n个扇区
rd_disk_m_16:
; ---------------------------------
; eax = LBA 扇区号
; bx = 将数据读入的内存地址
; cx = 读入的扇区数

    mov esi, eax  ; 备份eax
    mov di, cx      ; 备份cx

; 读写硬盘
; 第一步: 设置要读取的扇区数量
    mov dx, 0x1f2
    mov al,cl
    out dx,al

    mov eax, esi    ; 恢复ax

; 第2步: 将LBA 地址存入 0x1f3 ~ 0x1f6
    ; LBA  7-0 位 写入端口 0x1f3
    mov dx, 0x1f3
    out dx, al

    ; LBA  15-8 位 写入端口 0x1f4
    mov cl,8
    shr eax, cl
    mov dx, 0x1f4
    out dx,al

    ; LBA  23-16 位 写入端口 0x1f5
    shr eax,cl
    mov dx, 0x1f5
    out dx,al

    shr eax,cl
    and al,0x0f     ; LBA 第24-27位
    or al, 0xe0     ; 设置0x1f6 7-4位为1110, 表示LBA 模式
    mov dx, 0x1f6
    out dx,al

; 第3步:  向0x1f7 端口写入读命令, 0x20
    mov dx, 0x1f7
    mov al, 0x20
    out dx, al

; 第4步: 检测硬盘状态
.not_ready:
    ;同一端口,写时表示写入命令字, 读时表示读入硬盘状态
    nop
    in al, dx
    and al, 0x88    ; 第4位为1表示硬盘控制器已经准备好数据
                    ; 第7位为1, 表示硬盘忙
    cmp al, 0x08
    jnz .not_ready

; 第5步: 从0x1f0 端口读取数据
    mov ax, di          ; di为要读取的扇区数
    mov dx, 256
    mul dx              ; 每个扇区512字节,每次读取一个字,共需 di*512/2 = di*256 次
    mov cx, ax
    mov dx, 0x1f0
.go_on_read:
    in ax,dx
    mov [bx],ax
    add bx,2
    loop .go_on_read
    ret

    times 510-($-$$) db 0
    db 0x55, 0xAA

; nasm -I include/  -o mbr.bin mbr2.S