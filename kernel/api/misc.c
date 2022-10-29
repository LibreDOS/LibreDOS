#include<stdbool.h>
#include<stdint.h>
#include<ptrdef.h>
#include<api/stack.h>
#include<api/chario.h>
#include<api/exep.h>
#include<bios/disk.h>

/* int 21h ah=0x00 / int 20h */
void abort_program(void) {
    /* TODO implement */
    for (;;);
}

/* int 0h */
void divide_error(void) {
    kputs("\r\nDivide overflow\r\n");
}

/* int 25h */
void disk_read(void) {
    last_sp->ax = bios_disk_read(last_sp->al, last_sp->dx, last_sp->cx, FARPTR(last_sp->ds,last_sp->bx));
}

/* int 26h */
void disk_write(void) {
    last_sp->ax = bios_disk_write(last_sp->al, last_sp->dx, last_sp->cx, FARPTR(last_sp->ds,last_sp->bx));
}
