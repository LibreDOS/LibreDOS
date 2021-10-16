#include<stdint.h>
#include<ptrdef.h>
#include<lib/klib.h>
#include<api/stack.h>

extern void int23_dispatch(void);

void abort(void) {
    for (;;);
}

void divide_error(void) {
    kputs("\r\nDivide overflow\r\n");
}

void disk_read(void) {
    kputs("int 25h: absolute disk read!");
}

void disk_write(void) {
    kputs("int 26h: absolute disk write!");
}
