#include <system.h>

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
