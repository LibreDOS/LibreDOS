extern data_end
extern bss_end

extern kmain

global _start

%define target_address 0x500
%define load_address 0x200000

section .text

bits 32
_start:
    mov esi, load_address
    mov edi, target_address
    mov ecx, bss_end - target_address
    rep movsb

    mov eax, .reloc
    jmp eax
  .reloc:

    ; exit protected mode
    lidt [real_mode_idt]
    lgdt [real_mode_gdt]

    jmp 0x08:.mode16
  bits 16
  .mode16:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov eax, cr0
    and al, 0xfe
    mov cr0, eax

    jmp 0x0000:.realmode

    ; legacy boot sector entrypoint signature
    align 2
    dd 0x55AA55AA
  .realmode:

    xor ax, ax
    mov ds, ax
    mov es, ax
;    mov fs, ax
;    mov gs, ax
    mov ss, ax
    mov sp, 0xfff0

    sti

    call kmain

section .rodata

align 16
real_mode_idt:
    dw 0x3ff
    dd 0

align 16
real_mode_gdt:
    dw .end - .start - 1
    dd .start

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

section .mb_header

align 4
multiboot_header:
    .magic dd 0x1BADB002
    .flags dd 0x00010000
    .checksum dd -(0x1BADB002 + 0x00010000)
    .header_addr dd load_address + (multiboot_header - target_address)
    .load_addr dd load_address
    .load_end_addr dd load_address + (data_end - target_address)
    .bss_end_addr dd load_address + (bss_end - target_address)
    .entry_addr dd load_address + (_start - target_address)
