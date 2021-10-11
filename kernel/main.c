#include <stdint.h>
#include <bios/io.h>
#include <bios/disk.h>
#include <lib/klib.h>

void kmain(void) {
    char *buf;
    char __far *seg;
    //long i;
    //int c;

    kputs("Welcome to LibreDOS!\r\n");

    kputs("\r\nInitialising kernel data seg mem allocation system...");
    init_knalloc();
    kputs("  DONE");

    bios_init();

    kputs("\r\nAllocating 256 bytes...");
    buf = knalloc(256);
    kputs("  DONE");

    kputs("\r\nknalloc returned ptr = ");
    kprn_x((unsigned int)buf);

    kputs("\r\nAllocating 256 bytes...");
    buf = knalloc(256);
    kputs("  DONE");

    kputs("\r\nknalloc returned ptr = ");
    kprn_x((unsigned int)buf);

    kputs("\r\nInitialising mem seg allocation system...");
    init_kfalloc();
    kputs("  DONE");

    kputs("\r\nAllocating 128k bytes...");
    seg = kfalloc(131072,8);
    kputs("  DONE");

    kputs("\r\nkfalloc returned seg = ");
    kprn_x(SEGMENTOF(seg));

    kputs("\r\nAllocating 128k bytes...");
    seg = kfalloc(131072,8);
    kputs("  DONE");

    kputs("\r\nkfalloc returned seg = ");
    kprn_x(SEGMENTOF(seg));

    for (;;) {
        kputs("\r\nLibreDOS> ");
        kgets(buf, 256);
        kputs(buf);
    }
}
