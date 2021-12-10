%include "boot.inc"
SECTION loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP equ LOADER_BASE_ADDR

    jmp loader_start
;构建GDT 及其内部的描述符
    GDT_BASE dd 0x00000000  ;第一个描述符为无效的
             dd 0x00000000

    CODE_DESC dd 0x0000FFFF
              dd DESC_CODE_HIGH4
    DATA_STACK_DESC dd 0x0000FFFF
                    dd DESC_DATA_HIGH4

    VIDEO_DESC dd 0x80000007  ; limit=(0xbffff-0xb8000)/4k = 7
               dd DESC_VIDEO_HIGH4  ;DPL=0

    GDT_SIZE equ  $ - GDT_BASE
    GDT_LIMIT equ  GDT_SIZE -1
    times 60 dq 0      ; 预留60个描述符的空位

    ; total_men_bytes用于保存内存容量,以字节为单位. 此位置比较好记
    ; 当前偏移地址loader.bin是0x200. 60 * 8 + 4*8= 512(0x200)
    ; 故total_mem_bytes在内存中的地址是 0x900 + 0x200 = 0xb00
    total_mem_bytes dd 0


    SELECTOR_CODE  equ (0x0001 << 3) + TI_GDT + RPL0 ;相当于 (CODE_DESC - GDT_BASE)/8 + TI_GDT + RPL0
    SELECTOR_DATA  equ (0x0002 << 3) + TI_GDT + RPL0
    SELECTOR_VIDEO equ (0x0003 << 3) + TI_GDT + RPL0

    ; gdt的指针,前两字节是gdt的界限, 后4字节是gdt的起始位置
    gdt_ptr  dw  GDT_LIMIT
             dd  GDT_BASE

    ; 人工对齐: total_mem_bytes 4 + gdt_ptr6 + ards_buf 244 + ards_nr 2 = 256
    ; 用于记录 ARDS 结构体
    ards_buf times 244 db 0
    ards_nr  dw 0           ; 用于记录ARDS 结构体数量

loader_start:
    ;int 15h eax=000E820h , edx=534D4150h('SMAP') 获取内存布局
    xor  ebx, ebx    ;
    mov  edx, 0x534d4150
    mov  di, ards_buf   ; ards_buf  缓冲区结构
.e820_mem_get_loop:
    mov eax, 0x0000e820 ;执行int 0x15 后, eax值变为 0x534d4150,所以每次执行int都要更新为子功能号
    mov ecx, 20        ; ARDS 地址范围描述符大小是20字节
    int 0x15
    jc .e820_failed_so_try_e801 ; CF位为1, 则有错误发生, 尝试0xe801 子功能
    add di, cx          ; 使di增加20字节指向缓冲区中 新的ARDS结构
    inc word [ards_nr]  ; 记录ARDS 数量
    cmp  ebx, 0         ; 若ebx为0 且 cf 不为1, 这说明ards 全部返回,当前已是最后一个

    jnz .e820_mem_get_loop

    ; 在所有ARDS结构中,找出 (base_add_load + length_low)的最大值,即内存的容量
    mov  cx, [ards_nr]
    mov  ebx, ards_buf
    xor  edx, edx       ; edx为最大内容容量, 先清0
.find_max_mem_area:
    ; 无需判断type是否为1, 最大的内存块一定是可用内存
    mov  eax, [ebx]         ; base_add_low
    add  eax, [ebx+8]       ; length_low
    add  ebx, 20            ; 指向缓冲区中下一个ARDS 结构
    cmp  edx, eax           ; 冒泡排序,找出最大
    jge .next_ards
    mov edx, eax
.next_ards:
    loop .find_max_mem_area
    jmp .mem_get_ok

; ---- int 15h ax=E801 获取内存大小 最大支持4G
; 返回后, ax cx值一样,以KB为单位, bx dx 值一样, 以64KB 为单位
; 在ax 和cx寄存器中为低16M, bx和dx中为16MB 到4GB
.e820_failed_so_try_e801:
    mov ax, 0xe801
    int 0x15
    jc .e801_failed_so_try88

; 1. 先算出低15M 内存
; ax cx 中以kb为单位
    mov cx, 0x400       ; ax cx值一样, 将cx用作乘数
    mul cx
    shl edx, 16
    add  eax, 0x0000FFFF
    or  edx, eax
    add edx, 0x100000   ; ax只是15M, 故要加上1M
    mov esi, eax        ; 先把低15M 内存容量存入 esi 寄存器备份


; 再将16MB以上的内存转换为byte为单位
; 寄存器bx 为dx中以64KB为单位
    xor  eax, eax
    mov ax, bx
    mov ecx, 0x10000        ; 0x10000 为64KB
    mul ecx         ; 32位乘法, 默认的被乘数是 eax, 积为64位
    add esi, eax    ; 高32位存入edx, 低32位存入eax

    mov edx, esi    ; 由此只能测出4GB以内内存, 故32位eax足够了
    jmp .mem_get_ok

; int 15  ah=0x88  获取内存大小, 只能获取64MB 之内
.e801_failed_so_try88:
    mov ah, 0x88
    int 0x15
    jc .error_hlt
    and eax, 0x0000ffff

    ; 16位乘法, 被乘数是ax, 积为32位, 积的高16位在dx中,低16位在ax中
    mov cx, 0x400
    mul cx
    shl edx, 16         ; dx移到高16位
    or edx, eax         ; 积的低16位组合到eax
    add edx, 0x100000   ; 0x88 只会返回1MB以上的内存,故实际内容要加上1MB

.mem_get_ok:
    mov [total_mem_bytes], edx

.error_hlt:
    mov byte  [gs:160], 'd'
    mov byte  [gs:162], 'o'
    mov byte  [gs:164], 'n'
    mov byte  [gs:166], 'e'
    jmp $

