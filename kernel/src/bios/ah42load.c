int bios_hdd_load_sector(int drive, unsigned int seg, void *buf, int sector) {
#asm
    push bp
    mov bp, sp
    push bx
    push cx
    push dx
    push si
    mov dl, [bp+4]
    mov ax, [bp+6]
    mov [target_segment], ax
    mov ax, [bp+8]
    mov [target_offset], ax
    mov ax, [bp+10]
    mov [lba_low], ax
    mov si, dap
    mov ah, #$42
    clc
    int #$13
    jc failure
    xor ax, ax
    jmp done
  failure:
    xor ax, ax
    inc ax
    jmp done
  dap:
    size: db 16
    zero: db 0
    nsectors: dw 1
    target_offset: dw 0
    target_segment: dw 0
    lba_low: dd 0
    lba_high: dd 0
  done:
    pop si
    pop dx
    pop cx
    pop bx
    pop bp
#endasm
}
