int xchg_es(int new_segment) {
#asm
    push bp
    mov bp, sp
    push bx
    mov bx, [bp+4]
    mov ax, es
    mov es, bx
    pop bx
    pop bp
#endasm
}

int xchg_ds(int new_segment) {
#asm
    push bp
    mov bp, sp
    push bx
    mov bx, [bp+4]
    mov ax, ds
    mov ds, bx
    pop bx
    pop bp
#endasm
}

void kmain(void) {
    char *videomem = 0x0000;

    xchg_ds(0xb800);

    videomem[0] = 'a';

    for (;;);
}
