; API entry points and some other interrupt vectors
extern io_stack
extern dsk_stack
extern divide_error
extern disk_read
extern disk_write

extern abort
extern getchar_echo
extern getchar_echo
extern putchar
extern aux_getchar
extern aux_putchar
extern prn_putchar
extern direct_io
extern direct_input
extern getchar_no_echo
extern puts
extern gets
extern con_status
extern con_flush

global int_stub
global int00
global int20
global int21
global int24
global int25
global int26
global call5
global int23_dispatch
global int24_dispatch
global last_sp

%define max_char 0x0C
%define max_cpm 0x24

section .text
cpu 8086
bits 16

; interrupt stub
int_stub:
    iret

; divide by zero interrupt
int00:
    cld
    push ax
    push bx
    push cx
    push dx
    push ds
    mov ax, ss ; ensure ds=ss for GCC-IA16 to be happy
    mov ds, ax
    call divide_error ; works as long as no global variables are accessed!
    pop ds
    pop dx
    pop cx
    pop bx
    pop ax
    int 23h
    iret

; terminate interrupt
int20:
    xor ah, ah ; emulate int 21h call

; main API entry
int21:
    cmp ah, (dispatch_table.end - dispatch_table) / 2 ; check if function call in range
    jb short .valid_call
  .invalid_call:
    xor al, al ; clear al, if not
    iret
  .valid_call:
    call create_frame
    test ah, ah ; use IOSTACK for functions 0x01 to 0x0C, and DSKSTACK for everything else
    jz short .dskstack
    cmp ah, max_char
    ja short .dskstack
  ; I/O stack
    push cs
    pop ss
    mov sp, io_stack
    jmp .dispatch
  .dskstack:
    push cs
    pop ss
    mov sp, dsk_stack
  .dispatch:
    sti ; interrupts can happen now
    mov al, ah ; convert function number to offset in dispatch table
    cbw ; works as long as function smaller than 0x80
    mov bx, ax
    shl bx, 1
    call near [bx+dispatch_table] ; dispatch the actual function (hopefully it's "void func(void)")
    call restore_frame
    iret

; call 5 entry
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
    mov ah, cl ; put function number in ah (ah is not used in 8080 machine translation)
    jmp int21.valid_call

; initial int 24h handler
int24:
    xor al, al
    iret

; absolute disk read
int25:
    call create_frame
    mov word [temp_vector], disk_read
    jmp disk_dispatch

; absolute disk write
int26:
    call create_frame
    mov word [temp_vector], disk_write

; uses temp_vector as jump vector
disk_dispatch:
    push cs ; use disk stack (of course)
    pop ss
    mov sp, dsk_stack
    sti ; interrupts can happen now
    call [temp_vector]
    call restore_frame
    retf

create_frame:
    pop word [cs:temp_vector]
    push es ; you know the drill
    push ds
    push bp
    push di
    push si
    push dx
    push cx
    push bx
    push ax
    mov bx, cs ; set ds and es
    mov ds, bx
    mov es, bx
    mov [last_ss], ss ; save user ss:sp
    mov [last_sp], sp
    cld ; clear direction flag
    jmp near [temp_vector] ; return

restore_frame:
    cli ; prevent interrupts from happening
    pop word [temp_vector]
    mov ss, [last_ss] ; restore user stack
    mov sp, [last_sp]
    pop ax ; restore registers
    pop bx
    pop cx
    pop dx
    pop si
    pop di
    pop bp
    pop ds
    pop es
    jmp near [cs:temp_vector] ; return

; do ctrl+c dispatch
int23_dispatch:
    call restore_frame ; restore user stack
    int 23h ; let the Ctrl+C handler decide
    jmp int21 ; DOS 1.x just repeats the system call (abort by clearing ah)

; do critical error dispatch
int24_dispatch:
    push bp
    mov bp, sp
    push bx
    push si
    push di
    mov ax, [bp+4] ; obtain register values
    mov di, [bp+6]
    cli ; prevent any silly things from happening (especially while using critical_sp)
    mov [critical_sp], sp ; save kernel sp
    mov ss, [last_ss] ; restore user sp
    mov sp, [last_sp]
    int 24h ; pass control to critical error handler
    mov cx, cs ; reset segment registers
    mov ds, cx
    mov es, cx
    mov ss, cx
    mov sp, [critical_sp]
    sti
    pop di
    pop si
    pop bx
    pop bp
    ret ; al already contains the return code!

section .rodata

; function dispatch table
dispatch_table:
    dw abort_program
    dw getchar_echo, putchar, aux_getchar, aux_putchar
    dw prn_putchar, direct_io, direct_input, getchar_no_echo
    dw puts, gets, con_status, con_flush
  .end:

section .bss

; storage for caller stack
last_sp resw 1
last_ss resw 1
; storage for kernel stack during critical error
critical_sp resw 1
; temporary storage for jump/return vectors
temp_vector resw 1
