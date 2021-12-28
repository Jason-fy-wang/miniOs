BUILD=build
ENTRY_POINT=0xc0001500
BOOTDISK=hd60M.img
FLAG=-I include/ -I lib/ -I lib/kernel/ -I shell/ -I kernel/ -I device/ -I thread/ -I ../lib/user/
CC=gcc
CFLAGS=-m32 -fno-stack-protector -Wall -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes -nostdinc
LD=ld
LDFLAG=-m elf_i386 -Map $(BUILD)/kernel.map -Ttext $(ENTRY_POINT) -e main
AS=nasm

LIB_SRC=lib/kernel/print.S
KERNEL_SRC=$(BUILD)/main.o $(BUILD)/print.o $(BUILD)/init.o $(BUILD)/interrupt.o 	\
			$(BUILD)/kernel.o $(BUILD)/timer.o $(BUILD)/debug.o $(BUILD)/bitmap.o   \
			$(BUILD)/memory.o $(BUILD)/string.o $(BUILD)/thread.o $(BUILD)/list.o	\
			$(BUILD)/switch.o $(BUILD)/console.o  $(BUILD)/sync.o $(BUILD)/keyboard.o \
			$(BUILD)/ioqueue.o $(BUILD)/tss.o $(BUILD)/process.o $(BUILD)/syscall.o	\
			$(BUILD)/syscall_init.o $(BUILD)/stdio.o $(BUILD)/stdio_kernel.o $(BUILD)/ide.o 	\
			$(BUILD)/fs.o $(BUILD)/dir.o $(BUILD)/file.o $(BUILD)/inode.o $(BUILD)/fork.o	\
			$(BUILD)/shell.o $(BUILD)/assert.o

$(BUILD)/mbr.bin: boot/mbr.S
	@echo "build mbr.bin"
	$(AS) $(FLAG) -o $@ $^

$(BUILD)/loader.bin: loader/loader.S
	@echo "build loader.bin"
	$(AS) $(FLAG) -o $@ $^

$(BUILD)/main.o: kernel/main.c
	(cd kernel; make)

$(BUILD)/print.o: $(LIB_SRC)
	(cd lib; make)

$(BUILD)/kernel.bin: $(KERNEL_SRC)
	$(LD) $(LDFLAG) -o $@  $^

$(BUILD)/timer.o:
	(cd device; make)

$(BUILD)/thread.o:
	(cd thread; make)

$(BUILD)/tss.o:
	(cd user; make)

$(BUILD)/fs.o:
	(cd fs; make)

$(BUILD)/shell.o:
	(cd shell; make)

.PHONY: clean disk dir boot movlib

movlib:
	(cd lib; make movlib)
	(cd kernel; make movlib)

boot:
	bochs -f ../bochrc.disk

dasm:
	ndisasm $(BUILD)/mbr.bin > $(BUILD)/mbr.asm
	ndisasm $(BUILD)/loader.bin > $(BUILD)/loader.asm
	xxd -u -a -g 1  $(BUILD)/mbr.bin > $(BUILD)/mbrbinoffset
	xxd -u -a -g 1  $(BUILD)/loader.bin > $(BUILD)/loaderbinoffset
	xxd -u -a -g 1  $(BUILD)/kernel.bin > $(BUILD)/kerneloffset

dir:
	if [ ! -d $(BUILD) ]; then mkdir $(BUILD); fi

disk:
	mv ../$(BOOTDISK) .
	dd if=$(BUILD)/mbr.bin of=$(BOOTDISK) bs=512 count=1 conv=notrunc
	dd if=$(BUILD)/loader.bin of=$(BOOTDISK) bs=512 seek=2 count=4 conv=notrunc
	dd if=$(BUILD)/kernel.bin of=$(BOOTDISK) bs=512 seek=9 count=200 conv=notrunc
	mv $(BOOTDISK) ../

clean:
	$(RM) -r build

bootsec:$(BUILD)/mbr.bin $(BUILD)/loader.bin $(BUILD)/timer.o $(BUILD)/thread.o $(BUILD)/tss.o $(BUILD)/fs.o $(BUILD)/shell.o $(BUILD)/main.o

all:dir bootsec $(BUILD)/kernel.bin

