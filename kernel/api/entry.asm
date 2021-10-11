; API entry points and some other interrupt vectors
extern int00

global int00_entry
global int01
global int03
global int04

section .text
cpu 8086
bits16

int00_entry:
    call int00
    iret

int01:
    iret

int03:
    iret

int04:
    iret
