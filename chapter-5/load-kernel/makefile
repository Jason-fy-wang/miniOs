BUILD=build
BOOTDISK=hd60M.img
FLAG=-I include/
CC=gcc
CFLAG=-m32 -fno-stack-protector
LD=ld
LDFLAG=-m elf_i386

$(BUILD)/mbr.bin: boot/mbr.S
	@echo "build mbr.bin"
	nasm $(FLAG) -o $@ $^

$(BUILD)/loader.bin: loader/loader.S
	@echo "build loader.bin"
	nasm $(FLAG) -o $@ $^
$(BUILD)/main.o: kernel/main.c
	$(CC) $(CFLAG) -c -o $@ $^

$(BUILD)/kernel.bin: $(BUILD)/main.o
	$(LD) $(LDFLAG) -Ttext 0xc0001500 -e main -o $@ $^

.PHONY: clean disk dir boot

boot:
	bochs -f ../../bochrc.disk

dasm:
	ndisasm $(BUILD)/mbr.bin > $(BUILD)/mbr.asm
	ndisasm $(BUILD)/loader.bin > $(BUILD)/loader.asm
	xxd -u -a -g 1  $(BUILD)/mbr.bin > $(BUILD)/mbrbinoffset
	xxd -u -a -g 1  $(BUILD)/loader.bin > $(BUILD)/loaderbinoffset
	xxd -u -a -g 1  $(BUILD)/kernel.bin > $(BUILD)/kerneloffset

dir:
	if [ ! -d $(BUILD) ]; then mkdir $(BUILD); fi

disk:
	#mv ../../$(BOOTDISK) .
	bximage -hd=10M  -mode=create -q $(BOOTDISK)
	dd if=$(BUILD)/mbr.bin of=$(BOOTDISK) bs=512 count=1 conv=notrunc
	dd if=$(BUILD)/loader.bin of=$(BOOTDISK) bs=512 seek=2 count=4 conv=notrunc
	dd if=$(BUILD)/kernel.bin of=$(BOOTDISK) bs=512 seek=9 count=200 conv=notrunc
	mv $(BOOTDISK) ../../

clean:
	$(RM) -r build $(BOOTDISK)

all:dir $(BUILD)/mbr.bin $(BUILD)/loader.bin $(BUILD)/kernel.bin