BUILD=../build
CC=gcc
CFLAG=-I .  -I kernel/ -I ../kernel -m32 -fno-stack-protector -Wall -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes
LD=ld
AS=nasm -f elf

#$(BUILD)/print.o: kernel/print.S
	#nasm -f elf -o $@ $^

%.o: %.c
	$(CC) $(CFLAG) -c -o $(BUILD)/$*.o  $<

%.o: %.S
	$(AS) -o $(BUILD)/$*.o $<

%.s: %.c
	$(CC) $(CFLAG) -S -o $(BUILD)/$*.s $<



SRC=$(subst .c,.o, $(wildcard *.c))
OBJ=$(subst .S,.o, $(wildcard *.S))
ASM=$(subst .c,.s, $(wildcard *.c))

all: $(SRC) $(OBJ) lib/kernel
	@echo $(SRC)
	@echo $(OBJ)

lib/kernel:
	(cd kernel; make)

.PHONY: movlib clean

clean:
	rm -f *.o *.s

movlib:
	(cd kernel; make movlib)
	-mv *.o $(BUILD)
	-mv *.s $(BUILD)