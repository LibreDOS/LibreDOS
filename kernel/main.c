#include <stddef.h>
#include <stdint.h>
#include <ptrdef.h>
#include <bios/io.h>
#include <bios/disk.h>
#include <api/chario.h>
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
    //char far *seg;

    kputs("Welcome to LibreDOS!\r\n");

    //kputs("Initializing eternal heap allocation...\r\n");
    init_knalloc();

    kputs("Initializing I/O ...\r\n");
    bios_init();
    kputs("Initializing disks ...\r\n");
    bios_disk_init();

    //kputs("Initializing Interrupts");
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
    //kputs("  DONE\r\n");

    kputs("Allocating 256 bytes...");
    buf = knalloc(256);
    kputs("  DONE\r\n");

    kputs("knalloc returned ptr = ");
    kprn_x((unsigned int)buf);

    kputs("\r\nAllocating 256 bytes...");
    buf = knalloc(256);
    kputs("  DONE\r\n");

    kputs("knalloc returned ptr = ");
    kprn_x((unsigned int)buf);

    /*kputs("\r\nInitialising dynamic segment allocation...");
    init_kfalloc();
    kputs("  DONE\r\n");

    kputs("Allocating 128k bytes...");
    seg = kfalloc(131072, 8);
    kputs("  DONE");

    kputs("kfalloc returned seg = ");
    kprn_x(SEGMENTOF(seg));

    kputs("\r\nAllocating 128k bytes...");
    seg = kfalloc(131072, 8);
    kputs("  DONE\r\n");

    kputs("kfalloc returned seg = ");
    kprn_x(SEGMENTOF(seg));*/

    buf[0] = 254;
    buf[1] = 0;
    for (;;) {
        kputs("\r\nLibreDOS> ");
        asm volatile ("movw %%bx, %%ds\n"
                      "int $0x21" :: "a" (0x0C0A), "b" (0), "d" (buf) : "%ds");
        buf[2+buf[1]] = '\0';
        kputs("\n");
        kputs(buf+2);
    }
}
