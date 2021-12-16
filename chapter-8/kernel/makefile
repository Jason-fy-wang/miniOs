BUILD=../build
CC=gcc -m32
CFLAG=-I ../include/ -I ../lib/ -I ../lib/kernel/ -I ../device -I . -nostdinc
LD=ld
AS=nasm -f elf

#$(BUILD)/intrhandle.o: kernel.S
	#$(AS) -f elf -o $@ $^

%.o: %.c
	$(CC) $(CFLAG) -c -o $(BUILD)/$*.o  $<

%.o: %.S
	$(AS) -o $(BUILD)/$*.o $<

%.s: %.c
	$(CC) $(CFLAG) -S -o $(BUILD)/$*.s $<

SRC=$(subst .c,.o, $(wildcard *.c))
OBJ=$(subst .S,.o, $(wildcard *.S))
ASM=$(subst .c,.s, $(wildcard *.c))

all: $(SRC) $(OBJ)
	@echo $(SRC)
	@echo $(ASM)
	@echo $(OBJ)

.PHONY: movlib clean

clean:
	rm -f *.o *.s

movlib:
	-mv *.o $(BUILD)
	-mv *.s $(BUILD)


