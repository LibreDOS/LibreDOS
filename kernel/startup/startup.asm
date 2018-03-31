extern __edata
extern __end

global _main

section .text

bits 32
_main:
    mov byte [0xb8000], 'a'
    jmp $

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
