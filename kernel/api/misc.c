#include<stdint.h>
#include<ptrdef.h>
#include<lib/klib.h>
#include<api/stack.h>

void abort(void) {
    kprn_x(last_sp->ax);
    kprn_x(last_sp->bx);
    kprn_x(last_sp->cx);
    kprn_x(last_sp->dx);
    kputs("\r\nWell, shit went well!\r\n");
}
