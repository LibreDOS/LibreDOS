#include <stdint.h>
#include <ptrdef.h>
#include <bios/io.h>
#include <bios/disk.h>
#include <lib/klib.h>
#include <lib/alloc.h>

extern void int_stub(void);
extern void int00(void);
extern void int20(void);
extern void int21(void);
extern void int24(void);
extern void int25(void);
extern void int26(void);

void kmain(void) {
    char *buf;
    char far *seg;

    kputs("Welcome to LibreDOS!\r\n");

    kputs("\r\nInitialising kernel data seg mem allocation system...");
    init_knalloc();
    kputs("  DONE");

    kputs("\r\nInitializing I/O ...");
    bios_init();

    kputs("\r\nInitializing Interrupts");
    asm volatile ("cli");
    ivt[0x20] = int20;
    ivt[0x21] = int21;
    ivt[0x23] = int_stub;
    ivt[0x24] = int24;
    ivt[0x25] = int25;
    ivt[0x26] = int26;
    ivt[0] = int00;
    ivt[1] = int_stub;
    ivt[3] = int_stub;
    ivt[4] = int_stub;
    asm volatile ("sti");

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
    seg = kfalloc(131072, 8);
    kputs("  DONE");

    kputs("\r\nkfalloc returned seg = ");
    kprn_x(SEGMENTOF(seg));

    kputs("\r\nAllocating 128k bytes...");
    seg = kfalloc(131072, 8);
    kputs("  DONE");

    kputs("\r\nkfalloc returned seg = ");
    kprn_x(SEGMENTOF(seg));

    for (;;) {
        kputs("\r\nLibreDOS> ");
        kgets(buf, 256);
        kputs(buf);
    }
}
