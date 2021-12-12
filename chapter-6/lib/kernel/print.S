TI_GDT equ 0
RPL0 equ 0
SELECTOR_VIDEO equ (0x0003 << 3) + TI_GDT + RPL0

[bits 32]
;-----------------------put char-----------------------------------------
;  功能: 把栈中的1个字符写入光标所在处
;-----------------------------
global put_char
put_char:
    pushad   ; 备份32位寄存器环境
            ;pushad (push all double)入栈顺序为: EAX->ECX->EDX->EBX->ESP->EBP->ESI->EDI
    ; 保证gs为正确的视频段选择子
    mov ax, SELECTOR_VIDEO
    mov gs, ax

;; 获取当前光标位置
    ; 高8位
    mov dx, 0x03d4
    mov al, 0x0e
    out dx, al
    mov dx, 0x03d5
    in al, dx
    mov ah, al

    ; 低 8位
    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al
    mov dx, 0x03d5
    in al, dx

    ; 光标位置存入bx
    mov bx,ax

    ; 从栈中获取待打印的字符
    mov ecx, [esp + 36] ;pushad压入 4*8=32字节, 加上主调函数4字节返回地址,如为36字节

    cmp cl, 0x0d        ; CR(回车符) 是0xd, LF(换行符)是0a,  特殊字符的处理
    jz .is_carriage_return
    cmp cl, 0x0a
    jz .is_line_feed

    cmp cl, 0x8         ;BS（backspace)是8
    jz .is_backspace
    jmp .put_other

.is_backspace:
;当为backspace时,本质上只要将光标向前移动一个显存位置就可以,后面再输入的字符自然会覆盖此处的字符
;但有可能在键入backspace后不再输入新的字符,这时光标已经向前移动到待删除的字符位置,在字符还在原处
;这就有可能显得怪异,所以添加了空格或空字符0
    dec bx
    shl bx, 1

    mov byte [gs:bx], 0x20
    inc bx
    mov byte [gs:bx], 0x07
    shr bx, 1
    jmp .set_cursor

.put_other:
    shl bx, 1   ; 对应显存的偏移字节数
    mov [gs:bx], cl     ; 字符本身
    inc bx
    mov byte [gs:bx], 0x07  ; 字符属性
    shr bx, 1
    inc bx                  ;下一个字符位置
    cmp bx, 2000
    jl .set_cursor      ;若位超出2000,表示未写到显存最后,则去设置新的光标值. 如果超过屏幕大小,则换行处理

.is_carriage_return:        ; CR(\r)
.is_line_feed:              ;LF (\n)
    xor dx, dx              ; 被除数高16位,清0
    mov ax, bx              ; ax是被除数的低16位
    mov si, 80

    div si
    sub bx, dx              ; 光标值减去除80的余数,便是取整

.is_carriage_return_end:
    add bx, 80              ; \n 则把光标值 +80 即可
    cmp bx, 2000

.is_line_feed_end:
    jl .set_cursor

; 屏幕范围是0-24, 滚屏原理是将屏幕低1-24移动到0-23,再将24行用空格填充
.roll_screen:
    cld
    mov ecx, 960    ;2000-80=1920, 共需搬运 : 1920*2=3840字节.每次搬运4字节,需要 3840/4 = 960次
    mov esi, 0xc00b80a0     ;第一行行首
    mov edi, 0xc00b8000     ;第0行行首
    rep movsd

;将最后一行填充为空白
    mov ebx, 3840
    mov ecx, 80

.cls:
    mov word [gs:ebx], 0x0720       ; 黑底白字的空格
    add ebx, 2
    loop .cls
    mov bx, 1920        ; 将光标值重置为1920,最后一行行首

.set_cursor:
; 设置高8位
    mov dx, 0x03d4
    mov al, 0x0e
    out dx, al
    mov dx, 0x03d5
    mov al, bh      ;bh 是bx的高8位
    out dx, al

; 设置低8位
    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al
    mov dx, 0x03d5
    mov al, bl          ;bl 是bx的低8位
    out dx, al
.put_char_done:
    popad
    ret


