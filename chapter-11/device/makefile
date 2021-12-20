BUILD=../build
CC=gcc -m32
CFLAG=-I ../include/ -I ../lib/ -I ../lib/kernel/ -I ../device -I . -I ../thread/ -I ../kernel -fno-stack-protector -Wall -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes
LD=ld
AS=nasm -f elf

%.o: %.c
	$(CC) $(CFLAG) -c -o $(BUILD)/$*.o  $<

%.o: %.S
	$(AS) -o $(BUILD)/$*.o $<

%.s: %.c
	$(CC) $(CFLAG) -S -o $(BUILD)/$*.s $<

SRC=$(subst .c,.o, $(wildcard *.c))
OBJ=$(subst .S,.o, $(wildcard *.S))
ASM=$(subst .c,.s, $(wildcard *.c))

all:$(SRC) $(OBJ)
	@echo $(SRC)
	@echo $(OBJ)

.PHONY: movlib clean

clean:
	rm -f *.o *.s

movlib:
	-mv *.o $(BUILD)
	-mv *.s $(BUILD)