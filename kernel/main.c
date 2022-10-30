#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <ptrdef.h>
#include <bios/io.h>
#include <bios/disk.h>
#include <api/chario.h>
#include <api/stack.h>
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
    struct stack_frame_t stack_frame;
    static const char *hex_to_ascii_tab = "0123456789abcdef";
    struct bpb_t bpb;
    last_sp = &stack_frame;

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
        extern void gets(void);
        unsigned int i = 3;
        uint8_t far *location;
        uint16_t segment, offset, drive, start, count;
        union disk_change_t error;

        kputs("\r\nLibreDOS> ");
        stack_frame.ax = 0x0C0A;
        stack_frame.dx = (uint16_t)buf;
        stack_frame.ds = 0;
        gets();
        buf[2+buf[1]] = '\0';
        kputchar('\n');
        switch (buf[2]) {
            case 'd':
                segment = kread_hex(buf,&i);
                offset = kread_hex(buf,&i);
                count = kread_hex(buf,&i);
                location = FARPTR(segment,offset);
                for (i = 0; i < count; i++) {
                    kputchar(hex_to_ascii_tab[location[i] >> 4]);
                    kputchar(hex_to_ascii_tab[location[i] & 0x0F]);
                    if ((i & 0xF) == 0xF)
                        kputs("\r\n");
                    else
                        kputchar(' ');
                }
                break;
            case 'r':
                drive = kread_hex(buf,&i);
                start = kread_hex(buf,&i);
                count = kread_hex(buf,&i);
                segment = kread_hex(buf,&i);
                offset = kread_hex(buf,&i);
                if (bios_disk_read(drive,&error.error_code,start,count,FARPTR(segment,offset)))
                    kputs("success!");
                else {
                    kputs("error: ");
                    kprn_x(error.error_code);
                }
                break;
            case 'w':
                drive = kread_hex(buf,&i);
                start = kread_hex(buf,&i);
                count = kread_hex(buf,&i);
                segment = kread_hex(buf,&i);
                offset = kread_hex(buf,&i);
                if (bios_disk_write(drive,&error.error_code,start,count,FARPTR(segment,offset)))
                    kputs("success!");
                else {
                    kputs("error: ");
                    kprn_x(error.error_code);
                }
                break;
            case 'b':
                drive = kread_hex(buf,&i);
                if (bios_disk_build_bpb(drive,&error.error_code,&bpb)) {
                    kputs("bytes per sector: ");
                    kprn_ul(bpb.bytes_per_sector);
                    kputs("\r\nsectors per cluster: ");
                    kprn_ul(bpb.sectors_per_cluster);
                    kputs("\r\nreserved sectors: ");
                    kprn_ul(bpb.reserved_sectors);
                    kputs("\r\nFAT count: ");
                    kprn_ul(bpb.fat_count);
                    kputs("\r\nroot entries: ");
                    kprn_ul(bpb.root_entries);
                    kputs("\r\nsector count: ");
                    kprn_ul(bpb.sector_count);
                    kputs("\r\nmedia descriptor: ");
                    kprn_x(bpb.media_descriptor);
                    kputs("\r\nsectors per FAT: ");
                    kprn_ul(bpb.sectors_per_fat);
                } else {
                    kputs("error: ");
                    kprn_x(error.error_code);
                }
                break;
            case 'c':
                drive = kread_hex(buf,&i);
                if (bios_disk_change(drive,&error)) {
                    switch (error.change_status) {
                    case didnt_change:
                        kputs("didn't change");
                        break;
                    case dont_know:
                        kputs("don't know");
                        break;
                    case changed:
                        kputs("changed");
                        break;
                    }
                } else {
                    kputs("error: ");
                    kprn_x(error.error_code);
                }
                break;
        }
    }
}
