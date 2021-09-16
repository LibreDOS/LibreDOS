#include <bios/io.h>
#include <bios/disk.h>
#include <lib/klib.h>

void kmain(void) {
    char *buf;
    long i;
    int c;

    bios_puts("Welcome to LibreDOS!\r\n");

    bios_puts("\r\nInitialising kernel mem allocation system...");
    init_kalloc();
    bios_puts("  DONE");

    bios_puts("\r\nAllocating 256 bytes...");
    buf = kalloc(256);
    bios_puts("  DONE");

    bios_puts("\r\nkalloc returned ptr = ");
    kprn_x((unsigned long)buf);

    for (;;) {
        bios_puts("\r\nLibreDOS> ");
        bios_gets(buf, 256);
        bios_puts(buf);
    }
}
