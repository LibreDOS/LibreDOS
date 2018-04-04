#include <bios_io.h>
#include <bios_disk.h>
#include <klib.h>

void kmain(void) {
    char buf[256];
    long i;
    int c;

    bios_puts("Welcome to LibreDOS!\r\n");

    for (i = 0; i < 512; i++) {
        c = bios_read_byte(0x00, i);
        if (c == -1) {
            bios_puts("\r\nNo disk in A:.");
            break;
        }
        bios_putchar(c);
    }

    for (i = 0; i < 512; i++) {
        c = bios_read_byte(0x01, i);
        if (c == -1) {
            bios_puts("\r\nNo disk in B:.");
            break;
        }
        bios_putchar(c);
    }

    for (i = 0; i < 512; i++) {
        c = bios_read_byte(0x80, i);
        if (c == -1) {
            bios_puts("\r\nNo disk in C:.");
            break;
        }
        bios_putchar(c);
    }

    for (;;) {
        bios_puts("\r\nLibreDOS> ");
        bios_gets(buf, 256);
        bios_puts(buf);
    }
}
