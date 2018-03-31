#include <bios_io.h>

void bios_putchar(char c) {
#asm
    push bp
    mov bp, sp
    push ax
    mov ax, [bp+4]
    mov ah, #$0e
    int #$10
    pop ax
    pop bp
#endasm
}

void bios_puts(char *str) {
    int i;

    for (i = 0; str[i]; i++) {
        bios_putchar(str[i]);
    }

    return;
}

int bios_getchar(void) {
#asm
    xor ax, ax
    int #$16
    mov ah, #$0e
    int #$10
    mov al, ah
    xor ah, ah
#endasm
}
