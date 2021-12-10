; loader 加载的地址,以及 loader所在的扇区位置
LOADER_BASE_ADDR equ  0x900
LOADER_START_SECTOR equ 0x2
BIOSENTRY equ 0x7c00

; gdt 描述符属性
DESC_G_4K equ         1000_0000_0000_0000_0000_0000b
DESC_D_32 equ          100_0000_0000_0000_0000_0000b
DESC_L    equ           00_0000_0000_0000_0000_0000b    ;64位代码标记,此处标记为0
DESC_AVL  equ            0_0000_0000_0000_0000_0000b    ; 暂时不用,标记为0
DESC_LIMIT_CODE2  equ      1111_0000_0000_0000_0000b    ; code 段的limit高位设置为1111
DESC_LIMIT_DATA2  equ      1111_0000_0000_0000_0000b    ; data 段的limit高位设置为1111
DESC_LIMIT_VIDEO2 equ      0000_0000_0000_0000_1011b    ; 显存段
DESC_P            equ           1000_0000_0000_0000b    ; 是否在内存中
DESC_DPL_0        equ      000_0000_0000_0000b          ; 特权
DESC_DPL_1        equ      010_0000_0000_0000b
DESC_DPL_2        equ      100_0000_0000_0000b
DESC_DPL_3        equ      110_0000_0000_0000b
DESC_S_CODE       equ        1_0000_0000_0000b          ; 非系统段,代码为非系统段
DESC_S_SYS        equ        0_0000_0000_0000b          ; 系统段
DESC_S_DATA         equ        1_0000_0000_0000b          ; 数据为非系统段
DESC_TYPE_CODE    equ        1000_0000_0000b            ; 代码表示 可执行
DESC_TYPE_DATA    equ        0010_0000_0000b            ; 数据段 只读,向下扩展

DESC_CODE_HIGH4 equ (0x00<<24)+DESC_G_4K+DESC_D_32+DESC_L+DESC_AVL+DESC_LIMIT_CODE2+DESC_P +  \
                     DESC_DPL_0 + DESC_S_CODE + DESC_TYPE_CODE + 0x00

DESC_DATA_HIGH4 equ (0x00<<24)+DESC_G_4K+DESC_D_32+DESC_L+DESC_AVL+DESC_LIMIT_DATA2+DESC_P +  \
                    DESC_DPL_0+DESC_S_DATA+DESC_TYPE_DATA + +0x00

DESC_VIDEO_HIGH4 equ (0x00<<24)+DESC_G_4K+DESC_D_32+DESC_L+DESC_AVL+DESC_LIMIT_VIDEO2 + DESC_P + \
                     DESC_DPL_0 + DESC_S_DATA + DESC_TYPE_DATA + 0x00

; 选择子属性
RPL0   equ 00b
RPL1   equ 01b
RPL2   equ 10b
RPL3   equ 11b

TI_GDT equ 000b
TI_LDT equ 001b