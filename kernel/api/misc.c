#include<stdint.h>
#include<ptrdef.h>
#include<lib/klib.h>
#include<api/stack.h>

void abort(void) {
    kputs("abort!!!");//for (;;);
}

void divide_error(void) {
    kputs("\r\nDivide overflow\r\n");
    asm volatile ("int $0x20");
}

void disk_read(void) {
    kputs("int 25h: absolute disk read!");
}

void disk_write(void) {
    kputs("int 26h: absolute disk write!");
}
