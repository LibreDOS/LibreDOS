#include <bios_io.h>

void kmain(void) {
    char buf[256];

    bios_puts("Welcome to LibreDOS!");

    for (;;) {
        bios_puts("\r\nLibreDOS> ");
        bios_gets(buf, 256);
        bios_puts(buf);
    }
}
