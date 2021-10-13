; API entry points and some other interrupt vectors
extern io_stack
extern dsk_stack
extern divideError

global int00
global int01
global int03
global int04
global int20
global int21
global call5
global last_ss
global last_sp

%define max_cpm 0x24

section .text
cpu 8086
bits 16

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

int20:
    xor ah, ah ; emulate int 21h call

int21:
    cmp ah, (dispatch_table.end - dispatch_table) / 4 ; check if function call in range
    jb short .valid_call
  .invalid_call:
    xor al, al ; clear al, if not
    iret
  .valid_call:
    mov word [cs:setup_return], dispatch ; setup stack and dispatch function
    jmp setup_stack

call5:
    pop word [cs:last_ss] ; discard far call offset
    pop word [cs:last_ss] ; misuse last_ss to store far call segment
    pop word [cs:last_sp] ; misuse last_sp to store call 5 ip
    pushf ; simulate int 21h
    cli
    push word [cs:last_ss]
    push word [cs:last_sp]
    cmp cl, max_cpm ; see if it is one of the CP/M calls
    ja short int21.invalid_call
    mov word [cs:setup_return], .setfunc
    call setup_stack ; store everything
  .setfunc:
    mov ah, cl ; put function number in ah

dispatch:
    xor bx, bx ; set ds and es
    mov ds, bx
    mov es, bx
    mov al, ah ; convert function number to offset in dispatch table
    cbw ; works as long as function smaller than 0x80
    mov ax, bx
    shl bx, 1
    shl bx, 1
    call near [bx+dispatch_table] ; dispatch the actual function (hopefully it's "void func(void)")
    pop ax ; restore everything
    pop bx
    pop cx
    pop dx
    pop si
    pop di
    pop bp
    pop ds
    pop es
    cli
    mov ss, [cs:last_ss]
    mov sp, [cs:last_sp]
    iret

; make sure you set setup_return before jumping to this
setup_stack:
    mov [cs:last_ss], ss ; save user ss:sp
    mov [cs:last_sp], sp
    test ah, ah ; use IOSTACK for functions 0x01 to 0x0C, and DSKSTACK for everything else
    jz short .dskstack
    cmp ah, 0x0C
    ja short .dskstack
  ; I/O stack
    mov ss, [cs:kernel_cs]
    mov sp, io_stack
    jmp .createstack
  .dskstack:
    mov ss, [cs:kernel_cs]
    mov sp, dsk_stack
  .createstack:
    sti ; interrupts can now happen, since we set up the stack
    push es ; you know the drill
    push ds
    push bp
    push di
    push si
    push dx
    push cx
    push bx
    push ax
    jmp near [cs:setup_return] ; return

section .data

; function dispatch table
dispatch_table:
    dw 0x0000
  .end:

; storage for caller stack
last_ss dw 0x0000
last_sp dw 0x0000
; temporary storage for stack setup return address
setup_return dw 0x0000
; stores the kernel code segment
kernel_cs dw 0x0000
