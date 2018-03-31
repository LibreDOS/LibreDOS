#include <bios_io.h>
#include <bios_disk.h>

void kmain(void) {
    char buf[256];

    bios_puts("Welcome to LibreDOS!\r\n");

    for (;;) {
        bios_puts("\r\nLibreDOS> ");
        bios_gets(buf, 256);
        bios_puts(buf);
    }
}
