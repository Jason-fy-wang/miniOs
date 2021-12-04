BUILD=build
BOOTDISK=hd60M.img
FLAG=-I include/

$(BUILD)/mbr.bin: boot/mbr.S
	@echo "build mbr.bin"
	nasm $(FLAG) -o $@ $^

$(BUILD)/loader.bin: loader/loader.S
	@echo "build loader.bin"
	nasm $(FLAG) -o $@ $^

.PHONY: clean disk dir boot

boot:
	bochs -f ../bochrc.disk

dir:
	if [ ! -d $(BUILD) ]; then mkdir $(BUILD); fi

disk:
	dd if=$(BUILD)/mbr.bin of=$(BOOTDISK) bs=512 count=1 conv=notrunc
	dd if=$(BUILD)/loader.bin of=$(BOOTDISK) bs=512 seek=2 count=2 conv=notrunc

clean:
	$(RM) -r build

all:dir $(BUILD)/mbr.bin $(BUILD)/loader.bin