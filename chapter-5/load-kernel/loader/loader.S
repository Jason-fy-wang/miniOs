%include "boot.inc"
SECTION loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP equ LOADER_BASE_ADDR

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

.error_hlt:
    mov byte  [gs:160], 'd'
    mov byte  [gs:162], 'o'
    mov byte  [gs:164], 'n'
    mov byte  [gs:166], 'e'
    jmp $


.mem_get_ok:
    mov [total_mem_bytes], edx

    ;------准备进入保护模式
    ; 1. 打开A20
    in al, 0x92
    or  al, 0000_0010b
    out 0x92, al

    ; 2. 加载GDT
    lgdt [gdt_ptr]

    ; 3. cr0 设置PE 位
    mov eax, cr0
    or eax, 0x00000001
    mov cr0, eax

    jmp dword SELECTOR_CODE:p_mode_start    ;刷新流水线,进入保护模式

[bits 32]
p_mode_start:
    mov ax, SELECTOR_DATA
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, LOADER_STACK_TOP
    mov ax, SELECTOR_VIDEO
    mov gs, ax

    ; load kernel
    mov eax, KERNEL_START_SECTOR    ;kernel所在扇区
    mov ebx, KERNEL_BIN_BASE_ADDR
    mov ecx, 200                    ;
    call rd_disk_m_32

    ;  创建 page table
    call setup_page

    ; 保存 gdt表, 存储到原来的位置
    sgdt [gdt_ptr]

    ;重新设置gdt 描述符, 使虚拟地址执行内核的第一个页表
    mov ebx, [gdt_ptr + 2]      ; 获取页表基址
    or  dword [ebx + 0x18 + 4], 0xc0000000  ;设置视频段的基址

    ; 使gdt的基址加上0xc0000000 使其成为内核所在的高地址
    add  dword [gdt_ptr + 2], 0xc0000000

    add esp, 0xc0000000     ; 栈指针映射到内核地址

    ; 把也目录地址赋值 CR3
    mov eax, PAGE_DIR_TABLE_POS
    mov cr3, eax

    ; 打开cr0的pg位
    mov eax, cr0
    or  eax, 0x80000000
    mov cr0, eax

    ; 启用分页后,重新加载gdt
    lgdt [gdt_ptr]

    jmp SELECTOR_CODE:enter_kernel

enter_kernel:
    call kernel_init
    mov esp, 0xc009f000
    jmp KERNEL_ENTRY_POINT


    ; 创建也目录以及 页表
    ; 看到函数是没有 . 的
setup_page:
    ;  把也目录占用的空间逐字节清0
    mov ecx, 4096
    mov esi, 0
.clear_page_dir:
    mov byte [PAGE_DIR_TABLE_POS + esi],0
    inc esi
    loop .clear_page_dir

    ;开始创建页目录项(PDE)
.create_pde:    ; page directory entry
    mov eax, PAGE_DIR_TABLE_POS
    add eax, 0x1000  ; 此是第一个页表的位置及属性
    mov ebx, eax     ; ebx为第一个页表地址

    ; 将页目录0 和 页目录0xc00(768) 都存为第一个页表的地址,每个页表表示4M内存
    or eax, PG_US_U|PG_RW_W | PG_P      ; US=1表示用户属性,所有特权级别都可以访问

    mov [PAGE_DIR_TABLE_POS+0x0], eax       ; 第一个页表的位置0x101000 即属性 7, 第一个页表:0x101007
    mov [PAGE_DIR_TABLE_POS+0xc00], eax

    ; 0xc00以上的目录项用于内核空间,也就是0xc000_0000 --- 0xffff_ffff共计1G属于内核
    ; 0x0 --- 0xbfff_ffff 3G 空间属于用户
    ; 使最后一个页目录项 指向页目录表自己
    sub eax, 0x1000
    mov [PAGE_DIR_TABLE_POS+4092], eax

    ; 创建 PTE(页表项)
    mov ecx, 256        ; 低端1M内存/4k = 256
    mov esi, 0
    mov edx, PG_US_U|PG_RW_W|PG_P

.create_pte:
    mov [ebx + esi*4], edx      ; 映射低1M
    add edx, 4096
    inc esi
    loop .create_pte

; 创建内核其他页表的PDE
    mov eax, PAGE_DIR_TABLE_POS
    add eax, 0x2000                 ; 此时eax为第二个页表的位置
    or  eax, PG_US_U|PG_RW_W | PG_P
    mov ebx, PAGE_DIR_TABLE_POS
    mov ecx, 254            ; 范围为 769 - 1022 所有的目录项数量
    mov esi, 769
.create_kernel_pde:
    mov [ebx + esi *4], eax
    inc esi
    add eax, 0x1000
    loop .create_kernel_pde
    ret

; ------------------ 将kernel.bin中的segment 拷贝到编译的地址
kernel_init:
    xor eax, eax
    xor ebx, ebx    ;记录程序头表地址
    xor ecx, ecx    ; cx记录程序头中 program header 数量
    xor edx, edx    ; dx 记录program header 尺寸, 即e_phentsize

    mov dx, [KERNEL_BIN_BASE_ADDR+42]   ;偏移文件42字节处的属性是e_phentsize, 表示program header 大小
    mov ebx,[KERNEL_BIN_BASE_ADDR+28]   ;偏移文件开始部分28字节的地址是 e_phoff,表示第一个program header在文件中的偏移量

    add ebx, KERNEL_BIN_BASE_ADDR
    mov cx, [KERNEL_BIN_BASE_ADDR+44]    ; 偏移文件开始部分44字节的地方是e_phnum, 表示有几个program header

.each_segment:
    cmp byte [ebx+0], PT_NULL       ; 如果 p_type为PT_NULL ,说明此 program header 未使用
    je .PTNULL

    ; 为函数memcpy压入参数, 参数是从右往左顺序压入
    ; 函数原型: memcpy(dst, src, size)
    push dword [ebx + 16]   ; program header 偏移16字节的地方是p_filesize, 对应第三个参数 size
    mov eax, [ebx + 4]      ; 距程序头偏移量为4字节的位置为 p_offset
    add eax, KERNEL_BIN_BASE_ADDR   ; 加上kernel.bin被加载的物理地址,eax为该段的物理地址,即源地址
    push eax
    push dword [ebx + 8]    ; 目的地址. 偏移程序头8字节的位置是p_vaddr, 这就是目的地址
    call mem_cpy
    add esp, 12

.PTNULL:
    add ebx, edx        ;edx为program header的大小, 即e_phentsize.在此 ebx指向下一个program header
    loop .each_segment  ;
    ret

mem_cpy:
    cld
    push ebp
    mov ebp, esp
    push ecx    ; rep 使用到的ecx

    mov edi, [ebp + 8]  ; dst
    mov esi, [ebp + 12] ; src
    mov ecx, [ebp + 16] ; size
    rep movsb

    pop ecx
    pop ebp
    ret

; 保护模式下硬盘读取函数
;eax = LBA 扇区号
;bx = 将要读取的内存位置
;cx = 读入的扇区数
rd_disk_m_32:
    mov esi, eax
    mov di, cx

    ;第一步: 设置要读取的扇区数量
    mov dx, 0x1f2
    mov al, cl
    out dx, al

    mov eax, esi

    ; 第二步: 将LBA 地址存入 0x1f4-0x1f6
    mov dx, 0x1f3
    out dx, al

    mov cl, 8
    shr eax, cl
    mov dx, 0x1f4
    out dx, al

    shr eax, cl
    mov dx, 0x1f5
    out dx, al

    shr eax, cl
    and al, 0x0f
    or al, 0xe0
    mov dx, 0x1f6
    out dx, al

    ; 第三步: 向0x1f7 写入读取命令0x20
    mov dx, 0x1f7
    mov al, 0x20
    out dx, al

.not_ready:
    nop
    in al, dx
    and al, 0x88
    cmp al, 0x08
    jnz .not_ready

    mov ax, di
    mov dx, 256
    mul dx
    mov cx, ax
    mov dx, 0x1f0
.go_on_read:
    in ax, dx
    mov [ebx], ax
    add ebx, 2
    loop .go_on_read
    ret
