#include <stdint.h>
#include <bios/io.h>
#include <bios/disk.h>
#include <lib/klib.h>

void kmain(void) {
    char __far *buf;
    //long i;
    //int c;

    kputs("Welcome to LibreDOS!\r\n");

    kputs("\r\nInitialising kernel mem allocation system...");
    init_kalloc();
    kputs("  DONE");

    kputs("\r\nAllocating 256 bytes...");
    buf = kalloc(256);
    kputs("  DONE");

    kputs("\r\nkalloc returned ptr = ");
    kprn_x(SEGMENTOF(buf));

    kputs("\r\nAllocating 256 bytes...");
    buf = kalloc(256);
    kputs("  DONE");

    kputs("\r\nkalloc returned ptr = ");
    kprn_x(SEGMENTOF(buf));

    for (;;) {
        kputs("\r\nLibreDOS> ");
        kgets((char *)(SEGMENTOF(buf)<<4), 256);
        kputs((char *)(SEGMENTOF(buf)<<4));
    }
}
