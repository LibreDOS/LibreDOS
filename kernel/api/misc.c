#include<stdint.h>
#include<ptrdef.h>
#include<api/stack.h>
#include<api/chario.h>

/* int 21h ah=0x00 / int 20h */
void abort(void) {
    /* TODO implement */
    for (;;);
}

/* int 0h */
void divide_error(void) {
    kputs("\r\nDivide overflow\r\n");
}

/* int 25h */
void disk_read(void) {
    kputs("int 25h: absolute disk read!");
}

/* int 26h */
void disk_write(void) {
    kputs("int 26h: absolute disk write!");
}
