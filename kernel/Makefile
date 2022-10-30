CC = ia16-elf-gcc
LD = ia16-elf-gcc
ASM = nasm

CFLAGS ?= -pipe -O2 -Wall -Wextra -fpack-struct
LDFLAGS ?=

C_FILES = $(shell find . -type f -name '*.c')
ASM_FILES = $(shell find . -type f -name '*.asm')
OBJ = $(C_FILES:.c=.o) $(ASM_FILES:.asm=.o)

CHARDFLAGS = -ffreestanding -I.
LDHARDFLAGS = -nostdlib -lgcc -Tlinker.ld

.PHONY: clean

kernel.bin: $(OBJ) $(H_FILES)
	$(LD) $(OBJ) $(LDHARDFLAGS) $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CHARDFLAGS) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(ASM) $< -f elf32 -o $@

clean:
	rm -f $(OBJ) kernel.bin
