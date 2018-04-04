#include <klib.h>

void kprn_ul(unsigned long x) {
    int i;
    char buf[21];

    for (i = 0; i < 21; i++)
        buf[i] = 0;

    if (!x) {
        bios_putchar('0');
        return;
    }

    for (i = 19; x; i--) {
        buf[i] = (x % 10) + 0x30;
        x = x / 10;
    }

    i++;
    bios_puts(buf + i);

    return;
}

static char *hex_to_ascii_tab = "0123456789abcdef";

void kprn_x(unsigned long x) {
    int i;
    char buf[17];

    for (i = 0; i < 17; i++)
        buf[i] = 0;

    if (!x) {
        bios_puts("0x0");
        return;
    }

    for (i = 15; x; i--) {
        buf[i] = hex_to_ascii_tab[(x % 16)];
        x = x / 16;
    }

    i++;
    bios_puts("0x");
    bios_puts(buf + i);

    return;
}
