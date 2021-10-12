#include<ptrdef.h>
#include<lib/klib.h>

void divideError(void) {
    kputs("\r\nDivide overflow!\r\n");
    // TODO: make this abort the current program
    for (;;);
}
