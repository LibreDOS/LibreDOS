ASM = nasm

.PHONY: clean

bootsect.bin: bootsect.asm
	$(ASM) bootsect.asm -f bin -o bootsect.bin

clean:
	rm -f bootsect.bin
