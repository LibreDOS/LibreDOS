CC = bcc
LD = ld86
ASM = nasm

C_FILES = $(shell find ./src -type f -name '*.c')
H_FILES = $(shell find ./include -type f -name '*.h')
ASM_FILES = $(shell find ./src -type f -name '*.asm')
STARTUP_FILES = $(shell find ./startup -type f -name '*.asm')
OBJ = $(STARTUP_FILES:.asm=.o) $(C_FILES:.c=.o) $(ASM_FILES:.asm=.o)

CHARDFLAGS = -ansi -I./include/
LDHARDFLAGS = -d -T0x0010
CFLAGS = 
LDFLAGS = 

.PHONY: clean

kernel.bin: $(OBJ) $(H_FILES)
	$(LD) $(LDHARDFLAGS) $(OBJ) $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CHARDFLAGS) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(ASM) $< -f as86 -o $@

clean:
	rm -f $(OBJ) kernel.bin
