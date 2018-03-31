extern __edata
extern __end

extern _kmain

global _main

section .text

bits 32
_main:
    mov esi, trampoline - 0x10 + 0x100000
    mov edi, 0x1000
    mov ecx, trampoline.end - trampoline
    rep movsb

    ; exit protected mode
    lidt [real_mode_idt - 0x10 + 0x100000]
    lgdt [real_mode_gdt - 0x10 + 0x100000]

    mov ax, 0x0010
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x0008:0x1000

bits 16
  .realmode:
    sti
    call _kmain

bits 16
trampoline:
    mov eax, cr0
    and al, 0xfe
    mov cr0, eax

    xor eax, eax
    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx
    xor esi, esi
    xor edi, edi
    xor ebp, ebp
    xor esp, esp

    xor ax, ax
    mov fs, ax
    mov gs, ax
    not ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0xfff0

    jmp 0xffff:_main.realmode

  .end:

align 16
real_mode_idt:
    dw 0x3ff
    dd 0

align 16
real_mode_gdt:
    dw .end - .start - 1
    dd .start - 0x10 + 0x100000

align 16
    .start:

    ; Null descriptor (required)
    dw 0x0000           ; Limit
    dw 0x0000           ; Base (low 16 bits)
    db 0x00             ; Base (mid 8 bits)
    db 00000000b        ; Access
    db 00000000b        ; Granularity
    db 0x00             ; Base (high 8 bits)

    ; Real mode code
    dw 0xFFFF           ; Limit
    dw 0x0000           ; Base (low 16 bits)
    db 0x00             ; Base (mid 8 bits)
    db 10011010b        ; Access
    db 00000000b        ; Granularity
    db 0x00             ; Base (high 8 bits)

    ; Real mode data
    dw 0xFFFF           ; Limit
    dw 0x0000           ; Base (low 16 bits)
    db 0x00             ; Base (mid 8 bits)
    db 10010010b        ; Access
    db 00000000b        ; Granularity
    db 0x00             ; Base (high 8 bits)

    .end:

align 4
multiboot_header:
    .magic dd 0x1BADB002
    .flags dd 0x00010000
    .checksum dd -(0x1BADB002 + 0x00010000)
    .header_addr dd (multiboot_header - 0x10 + 0x100000)
    .load_addr dd 0x100000
    .load_end_addr dd (__edata - 0x10 + 0x100000)
    .bss_end_addr dd (__end - 0x10 + 0x100000)
    .entry_addr dd (_main - 0x10 + 0x100000)
