#include <stdint.h>
#include <ptrdef.h>
#include <api/chario.h>
#include <lib/klib.h>

void kprn_ul(unsigned long x) {
    int i;
    char buf[21];

    for (i = 0; i < 21; i++)
        buf[i] = 0;

    if (!x) {
        kputchar('0');
        return;
    }

    for (i = 19; x; i--) {
        buf[i] = (x % 10) + 0x30;
        x = x / 10;
    }

    i++;
    kputs(buf + i);

    return;
}

static char *hex_to_ascii_tab = "0123456789abcdef";

void kprn_x(unsigned long x) {
    int i;
    char buf[17];

    for (i = 0; i < 17; i++)
        buf[i] = 0;

    if (!x) {
        kputs("0x0");
        return;
    }

    for (i = 15; x; i--) {
        buf[i] = hex_to_ascii_tab[(x % 16)];
        x = x / 16;
    }

    i++;
    kputs("0x");
    kputs(buf + i);

    return;
}

void kpanic(char *str) {
    kputs(str);
    asm volatile ("cli");
    for (;;);
}

void *knmemcpy(void *dest, void *src, unsigned int count) {
    unsigned int i;
    char *destptr = dest;
    char *srcptr = src;

    for (i=0; i < count; i++)
        destptr[i] = srcptr[i];

    return dest;
}

void far *kfmemcpy(void far *dest, void far *src, unsigned long count) {
    unsigned int i,j;
    char far *destptr = dest;
    char far *srcptr = src;

    for (i = 0; i < (count >> 16); i++) {
        j = 0;
        do {
            destptr[j] = srcptr[j];
            j++;
        } while (j);
        destptr += (unsigned long)FARPTR(256,0);
        srcptr += (unsigned long)FARPTR(256,0);
    }

    for (i=0; i < (count & 0xFFFF); i++)
        destptr[i] = srcptr[i];

    destptr = dest;
    srcptr = src;
    return dest;
}
