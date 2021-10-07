#include <stdint.h>
#include <bios/io.h>

void bios_putchar(char c) {
    asm volatile ("int $0x10" :: "a" (0x0e00 | c), "b" (0) : "%bp");
}

void bios_puts(char *str) {
    int i;

    for (i = 0; str[i]; i++) {
        bios_putchar(str[i]);
    }

    return;
}

int bios_getchar(void) {
    uint16_t ret = 0;
    asm volatile ("int $0x16" : "+a" (ret));
    return ret & 0xff;
}

void bios_gets(char *str, int limit) {
    int i = 0;
    int c;

    for (;;) {
        c = bios_getchar();
        switch (c) {
            case 0x08:
                if (i) {
                    i--;
                    bios_putchar(0x08);
                    bios_putchar(' ');
                    bios_putchar(0x08);
                }
                continue;
            case 0x0d:
                bios_putchar(0x0d);
                bios_putchar(0x0a);
                str[i] = 0;
                break;
            default:
                if (i == limit - 1)
                    continue;
                bios_putchar(c);
                str[i++] = (char)c;
                continue;
        }
        break;
    }

    return;
}
