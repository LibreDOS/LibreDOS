#include <stdint.h>
#include <ptrdef.h>
#include <bios/disk.h>
#include <api/chario.h>
#include <lib/klib.h>

/* BIOS diskette parameter table
 * most fields included for completeness
 * only interest is sectors per track */
__attribute__((packed)) struct dpt_t {
    uint16_t specify;
    uint8_t motor_delay;
    uint8_t bytes_per_sector;
    uint8_t sectors_per_track;
    uint8_t sector_gap_length;
    uint8_t data_length;
    uint8_t formatting_gap_length;
    uint8_t format_filler_byte;
    uint8_t head_settle_time;
    uint8_t motor_start_time;
    /* IBM SecurePath BIOS (no idea what this is) */
    uint8_t maximum_track_number;
    uint8_t data_transfer_rate;
    uint8_t cmos_drive_type;
};

/* relevant fields in the BDA */
#define HDD_COUNT ((uint8_t *)0x475)
#define DISK_FLAG ((uint8_t *)0x504)
#define DOS_DPT ((struct dpt_t *)0x522)

#define MAX_DISK_COUNT ('Z'-'A'+1)

static struct {
    int bios_number;
    enum {
        none, floppy_no_detect, floppy_swap_detect, hard_drive
    } type;
    int cylinder_count, head_count, sector_count;
} disks[MAX_DISK_COUNT];

static int disk_count = 0;

static struct {
    int disk_number;
    uint32_t hidden_sectors;
} partitions[MAX_DISK_COUNT];

static int partition_count = 0;

static const char floppy_drive_types[6][8] = {{"unknown"},{"360k"},{"1220k"},
                                              {"720k"},{"1440k"},{"2880k"}};

void bios_disk_init(void) {
    /* get the amount of floppy drives */
    uint16_t equipment = 0;
    int i;
    /* try int 11h first */
    asm ("int $0x11" : "=a" (equipment));
    if (equipment & 1) {
        /* seems like we got something valid */
        disk_count = ((equipment >> 6) & 3) + 1;
        for (i = 0; i < disk_count; i++) {
            disks[i].bios_number = i;
            disks[i].type = floppy_no_detect;
        }
    }

    /* scan through floppy drives to get info from functions 8 and 15h */
    i = 0;
    do {
        /* call function 8 */
        uint16_t flags, floppy_type, cylinder_sector, head_drive;
        asm ("int $0x13\n"
             "pushf\n"
             "popw %%ax\n"
             "sti" : "=a" (flags), "=b" (floppy_type), "=c" (cylinder_sector), "=d" (head_drive)
             : "a" (0x0800), "d" (i) : "%si", "%di", "%bp", "%ds", "%es", "cc");
        asm volatile ("int $0x13" :: "a" (0x0100), "d" (i) : "cc");
        /* if carry flag isn't set and cylinder and sector count isn't zero */
        if (!(flags & 1) && cylinder_sector) {
            uint16_t drive_type;
            /* if we got zero drives from int 11h, but got something here, use that */
            if (!disk_count)
                disk_count = head_drive & 0xFF;
            /*disks[i].sector_count = cylinder_sector & 0x3F;
            disks[i].head_count = head_drive >> 8;
            disks[i].cylinder_count = (cylinder_sector >> 8) + ((cylinder_sector >> 6) & 3);*/
            /* convert drive type
             * 5 and 6 are treated as aliases for 2880k
             * everything beyond is treated as unknown */
            floppy_type &= 0xFF;
            if (floppy_type == 6)
                floppy_type--;
            else if (floppy_type > 6)
                floppy_type = 0;
            /* call function 15h */
            asm("int $0x13\n"
                "pushf\n"
                "popw %%bx" : "=a" (drive_type), "=b" (flags) : "a" (0x1500), "d" (i) : "%cx", "cc");
            asm volatile ("int $0x13" :: "a" (0x0100), "d" (i) : "cc");
            /* if carry clear */
            if (!(flags & 1)) {
                if (drive_type != 3)
                    drive_type >>= 8;
                if (drive_type != 0 && drive_type < 4)
                    disks[i].type = drive_type;
            }
        }
        else floppy_type = 0;
        if (disk_count) {
            kputs("Detected floppy drive ");
            kprn_ul(i);
            kputs(": ");
            kputs(floppy_drive_types[floppy_type]);
            if (disks[i].type != floppy_no_detect)
                kputs(", change detection");
            kputs("\r\n");
        }
        i++;
    } while (i < disk_count && i < 0x80);

    /* initialize floppy partitions */
    partition_count = disk_count;
    for (i = 0; i < disk_count; i++)
        partitions[i].disk_number = i;
    /* reserve drive letters A: and B: */
    if (partition_count == 0) {
        partition_count = 2;
        partitions[0].disk_number = -1;
        partitions[1].disk_number = -1;
    } else if (partition_count == 1) {
        partition_count = 2;
        /* if only one floppy drive present, emulate the second one */
        partitions[1].disk_number = 0;
    }
    /* reset flag for virtual floppy drive */
    *DISK_FLAG = 0;

    /* copy BIOS DPT and update interrupt vector */
    kfmemcpy(DOS_DPT, ivt[0x1E], sizeof(struct dpt_t));
    DOS_DPT->sectors_per_track = 36;
    ivt[0x1E] = DOS_DPT;
}
