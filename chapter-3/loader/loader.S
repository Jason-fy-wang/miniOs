%include "boot.inc"
SECTION loader vstart=LOADER_BASE_ADDR

; 输出loader
    mov byte [gs:0x00],'2'
    mov byte [gs:0x01], 0xA4
    mov byte [gs:0x02],''
    mov byte [gs:0x03], 0xA4
    mov byte [gs:0x04],'l'
    mov byte [gs:0x05], 0xA4
    mov byte [gs:0x06],'o'
    mov byte [gs:0x07], 0xA4
    mov byte [gs:0x08],'a'
    mov byte [gs:0x09], 0xA4
    mov byte [gs:0x0A],'d'
    mov byte [gs:0x0B], 0xA4
    mov byte [gs:0x0C],'e'
    mov byte [gs:0x0D], 0xA4
    mov byte [gs:0x0E],'r'
    mov byte [gs:0x0F], 0xA4

    jmp $

