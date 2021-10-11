extern key_code

global int1B

section .text
bits 16

int1B:
    mov byte [cs:key_code], 0x03 ; Throw a ^C in there
    iret
