BUILD=../build
CC=gcc
CFLAG=-I .  kernel/
LD=ld


$(BUILD)/print.o: kernel/print.S
	nasm -f elf -o $@ $^


all: $(BUILD)/print.o