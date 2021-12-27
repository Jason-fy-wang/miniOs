TI_GDT equ 0
RPL0 equ 0
SELECTOR_VIDEO equ (0x0003 << 3) + TI_GDT + RPL0
section .data
put_int_buffer  dq 0
[bits 32]
section .text
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

;-----------------------
; put_str 通过put_char来打印以0字符结尾的字符串
;-----------------------
; 输入:栈中参数为打印的字符串
; 输出: 无

global put_str
put_str:
    push ebx    ; 由于使用到了ebx和ecx寄存器,故备份此两个寄存器
    push ecx
    xor ecx, ecx
    mov ebx, [esp+12]
.goon:
    mov cl, [ebx]
    cmp cl, 0
    jz .str_over
    push ecx
    call put_char
    add esp, 4
    inc ebx
    jmp .goon

.str_over:
    pop ecx
    pop ebx
    ret

;-------------------- 清屏操作
global cls_screen
cls_screen:
    pushad
    mov ax, SELECTOR_VIDEO  ; 显存选择子
    mov gs, ax

    mov ebx,0
    mov ecx, 80*25
.cls:
    mov word [gs:ebx], 0x0720   ; 0x720是黑底白字的空格键
    add ebx, 2
    loop .cls
    mov ebx, 0
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
    popad
    ret
; --------------------------------将小端字节序的数字变成对应的ASCII后,倒置
; 输入: 栈中参数为待打印的数字
; 输出: 在屏幕是哪个打印16位进制数字, 并不会打印前缀0x
; 如打印10进制15时, 只会打印f
;--------------------------------------------------------
global put_int
put_int:
    pushad
    mov ebp, esp
    mov eax, [ebp+4*9]      ; call的返回地址占4字节+pushad的8个4字节
    mov edx, eax
    mov edi, 7
    mov ecx, 8
    mov ebx, put_int_buffer
.16based_4bits:
    ;将32位数字按照16进制的形式从低位到高位逐个处理,共处理8个16进制数组
    and edx, 0x0000000f
    cmp edx, 9
    jg  .is_A2F
    add edx, '0'
    jmp .store

.is_A2F:
    sub edx, 10
    add edx, 'A'

.store:
    mov [ebx+edi], dl
    dec edi
    shr eax, 4
    mov edx, eax
    loop .16based_4bits
.ready_to_print:
    inc edi

.skip_prefix_0:
    cmp edi, 8

    je .full0
.go_to_skip:
    mov cl, [put_int_buffer+edi]
    inc edi
    cmp cl,'0'
    je .skip_prefix_0
    dec edi
    jmp .put_each_num

.full0:
    mov cl,'0'

.put_each_num:
    push ecx
    call put_char
    add esp, 4
    inc edi
    mov cl, [put_int_buffer+edi]
    cmp edi, 8
    jl .put_each_num
    popad
    ret


global set_cursor
set_cursor:
    pushad
    mov bx, [esp + 36]
;;;; 设置高8位
    mov dx, 0x03d4
    mov al, 0x0e
    out dx, al
    mov dx, 0x03d5
    mov al, bh
    out dx, al

;;;; 设置低8位
    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al
    mov dx, 0x03d5
    mov al,bl
    out dx, al
    popad
    ret
