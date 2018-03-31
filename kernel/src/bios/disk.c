


int bios_floppy_load_sector_chs(int drive, unsigned int seg,
                                void *buf, int cyl, int head, int sect) {
#asm
    push bp
    mov bp, sp
    push bx
    push cx
    push dx
    mov ax, [bp+14]
    mov cl, al
    mov ax, [bp+12]
    mov dh, al
    mov ax, [bp+10]
    mov ch, al
    mov ax, [bp+8]
    mov bx, ax
    mov ax, [bp+6]
    mov es, ax
    mov ax, [bp+4]
    mov dl, al
    mov ah, #$02
    mov al, #$01
    int #$13
    jc .failure
  .success:
    xor ax, ax
    jmp .out
  .failure:
    xor ax, ax
    inc ax
  .out:
    pop dx
    pop cx
    pop bx
    pop bp
#endasm
}


int bios_floppy_load_sector(int drive, unsigned int seg,
                            void *buf, int sector) {



}
