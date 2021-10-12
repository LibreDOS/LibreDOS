; API entry points and some other interrupt vectors
extern divideError

global int00
global int01
global int03
global int04

section .text
cpu 8086
bits16

; divide by zero interrupt
int00:
    call divideError
    iret ; Execution shouldn't reach this point!

; single-step interrupt
int01:
    iret

; breakpoint interrupt
int03:
    iret

; overflow interrupt
int04:
    iret
