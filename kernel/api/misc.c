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
bool disk_read(void) {
    uint16_t error_code;
    bool success = bios_disk_read(last_sp->al, &error_code, last_sp->dx, last_sp->cx, FARPTR(last_sp->ds,last_sp->bx));
    last_sp->ax = error_code;
    return success;
}

/* int 26h */
bool disk_write(void) {
    uint16_t error_code;
    bool success = bios_disk_write(last_sp->al, &error_code, last_sp->dx, last_sp->cx, FARPTR(last_sp->ds,last_sp->bx));
    last_sp->ax = error_code;
    return success;
}
