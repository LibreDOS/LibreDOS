#include <bios_disk.h>

int bios_floppy_load_sector_chs(int drive, unsigned int seg,
                                void *buf, int cyl, int head, int sect) {
#asm
    push bp
    mov bp, sp
    push bx
    push cx
    push dx
    push es
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
    pop es
    pop dx
    pop cx
    pop bx
    pop bp
#endasm
}

#define     FLOPPY_HPC     2
#define     FLOPPY_SPT     18

int bios_floppy_load_sector(int drive, unsigned int seg,
                            void *buf, int sector) {
    /* convert LBA to CHS and call the CHS function */

    int cyl, head, sect;

    cyl = sector / (FLOPPY_HPC * FLOPPY_SPT);
    head = (sector / FLOPPY_SPT) % FLOPPY_HPC;
    sect = (sector % FLOPPY_SPT) + 1;

    return bios_floppy_load_sector_chs(drive, seg, buf, cyl, head, sect);
}


int bios_hdd_load_sector(int drive, unsigned int seg, void *buf, int sector) {
#asm
    push bp
    push si
    mov bp, sp
    push bx
    push cx
    push dx
    push es
    mov ah, #$42
    mov dl, [bp + 6]
    mov cx, [bp]
    mov [.lba_low], cx
    mov cx, [bp + 4]
    mov [.target_segment], cx
    xor si, si
    mov si, .dap

    clc

    int #$13

    jc .failed
   .succ:
    xor ax, ax
    jmp .exit
   .failed:
    xor ax, ax
    inc ax
   .exit:
    pop es
    pop dx
    pop cx
    pop bx
	pop si
    pop bp
   .dap:
    .size: db 16
    .zero: db 0
    .nsectors: dw 1
    .target_offset: dw 0
    .target_segment: dw 0
    .lba_low: dd 0
    .lba_high: dd 0
#endasm
}

int bios_load_sector(int drive, unsigned int seg,
                     void *buf, int sector) {

    if (drive < 0x80)
        return bios_floppy_load_sector(drive, seg, buf, sector);
    else
        return bios_hdd_load_sector(drive, seg, buf, sector);

}

