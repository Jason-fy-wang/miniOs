BUILD=build
BOOTDISK=hd60M.img
FLAG=-I include/ -I lib/ -I lib/kernel/ -I kernel/ -I device/
CC=gcc
CFLAG=-m32 -fno-stack-protector
LD=ld
LDFLAG=-m elf_i386
AS=nasm

LIB_SRC=lib/kernel/print.S
KERNEL_SRC=$(BUILD)/main.o $(BUILD)/print.o $(BUILD)/init.o $(BUILD)/interrupt.o $(BUILD)/kernel.o $(BUILD)/timer.o $(BUILD)/debug.o

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
	$(LD) $(LDFLAG) -Ttext 0xc0001500 -e main -o $@  $^

$(BUILD)/timer.o:
	(cd device; make)


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

bootsec:$(BUILD)/mbr.bin $(BUILD)/loader.bin $(BUILD)/timer.o $(BUILD)/main.o

all:dir bootsec $(BUILD)/kernel.bin

