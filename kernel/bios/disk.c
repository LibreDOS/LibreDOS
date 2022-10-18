#include <stdbool.h>
#include <stdint.h>
#include <ptrdef.h>
#include <bios/disk.h>
#include <api/chario.h>
#include <lib/klib.h>

/* relevant fields in the BDA */
#define HDD_COUNT ((uint8_t *)0x475)
#define DISK_FLAG ((uint8_t *)0x504)
#define DOS_DPT ((struct dpt_t *)0x522)

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


#define MAX_DISK_COUNT ('Z'-'A'+1)

static struct drive_t {
    int bios_number;
    enum {
        none, floppy_no_detect, floppy_swap_detect, hard_drive
    } type;
    enum {
        unknown, floppy_360, floppy_1220, floppy_720, floppy_1440, floppy_2880
    } media_type;
    int cylinder_count, head_count, sector_count;
} drives[MAX_DISK_COUNT];

static int drive_count = 0;
static bool virtual_drive = false;

static struct parition_t {
    int drive_number;
    uint32_t hidden_sectors;
} partitions[MAX_DISK_COUNT];

static int partition_count = 0;

static const char floppy_drive_types[6][8] = {{"unknown"},{"360k"},{"1220k"},
                                              {"720k"},{"1440k"},{"2880k"}};


struct int13_8_t {
    uint16_t flags, floppy_type, cylinder_sector, head_drive;
};

/* read info from function 8 */
static bool int13_8 (int drive, struct int13_8_t *data) {
    asm("int $0x13\n"
        "pushf\n"
        "popw %%ax\n"
        "sti" : "=a" (data->flags), "=b" (data->floppy_type),
        "=c" (data->cylinder_sector), "=d" (data->head_drive)
        : "a" (0x0800), "d" (drive) : "%si", "%di", "%bp", "%ds", "%es", "cc");
    asm volatile ("int $0x13" :: "a" (0x0100), "d" (drive) : "cc");
    /* check if carry flag not set and cylinder/sector number not zero */
    return !(data->flags & 1) && data->cylinder_sector;
}

struct int13_15_t {
    uint16_t flags, drive_type;
};

/* read info from function 15h */
static bool int13_15 (int drive, struct int13_15_t *data) {
    asm("int $0x13\n"
        "pushf\n"
        "popw %%bx" : "=a" (data->drive_type), "=b" (data->flags)
        : "a" (0x15FF), "d" (drive) : "%cx", "cc");
    asm volatile ("int $0x13" :: "a" (0x0100), "d" (drive) : "cc");
    /* guard against bug in AX return value */
    if (data->drive_type == 3)
        data->drive_type <<= 8;
    data->drive_type >>= 8;
    return !(data->flags & 1) && data->drive_type > 0 && data->drive_type < 4;
}


static void scan_drives(int start, int end, int count) {
    int detected_count = 0, i;
    bool use_int13_15 = false;
    struct int13_8_t int13_8_data;
    struct int13_15_t int13_15_data;

    /* get drive count from function 8, if zero was passed */
    for (i = start; i < end && count == 0; i++) {
        if (int13_8(i,&int13_8_data))
            count = int13_8_data.head_drive & 0xFF;
    }

    /* scan drive numbers for drives */
    for (i = start; i < end && (detected_count < count || !use_int13_15); i++) {
        bool int13_8_valid, int13_15_valid;
        struct drive_t drive;
        drive.bios_number = i;
        drive.type = floppy_no_detect;
        if ((int13_8_valid = int13_8(i,&int13_8_data))) {
            if (int13_8_data.floppy_type == 6)
                int13_8_data.floppy_type--;
            if (int13_8_data.floppy_type < 6)
                drive.media_type = int13_8_data.floppy_type;
            drive.sector_count = int13_8_data.cylinder_sector & 0x3F;
            drive.head_count = (int13_8_data.head_drive >> 8) + 1;
            drive.cylinder_count = (int13_8_data.cylinder_sector >> 8)
                                 + ((int13_8_data.cylinder_sector << 2) & 0x300)
                                 + 1;
        }
        if ((int13_15_valid = int13_15(i,&int13_15_data))) {
            /* if we find a drive number with valid function 15 output,
             * discard the drives with only valid function 8 output */
            if (!use_int13_15) {
                use_int13_15 = true;
                detected_count = 0;
            }
            drive.type = int13_15_data.drive_type;
        }
        /* accept drive if function 15 returns something valid
         * if only function 8 returns something valid, take that if nothing
         * works on function 15 (i.e. presumably not implemented) */
        if (detected_count < count && ((!use_int13_15 && int13_8_valid) || int13_15_valid))
            drives[drive_count + detected_count++] = drive;
    }

    drive_count += detected_count;
}

void bios_disk_init(void) {
    /* get the amount of floppy drives */
    uint16_t equipment = 0;
    int count = 0, i;
    /* try int 11h first */
    asm ("int $0x11" : "=a" (equipment));
    if (equipment & 1)
        count = ((equipment >> 6) & 3) + 1;
    /* scan for floppy drives */
    scan_drives(0,0x80,count);
    /* if nothing found, assume those functions weren't implemented yet */
    if (drive_count == 0) {
        for (i = 0; i < count; i++) {
            drives[i].bios_number = i;
            drives[i].type = floppy_no_detect;
        }
        drive_count = count;
    }

    /* detect hard drives */
    count = *HDD_COUNT;
    if (count != 0)
        scan_drives(0x80,0x100,count);

    /* initialize floppy partitions */
    partition_count = drive_count;
    for (i = 0; i < drive_count; i++)
        partitions[i].drive_number = i;
    /* reserve drive letters A: and B: */
    if (partition_count == 0) {
        partition_count = 2;
        partitions[0].drive_number = -1;
        partitions[1].drive_number = -1;
    } else if (partition_count == 1) {
        partition_count = 2;
        /* if only one floppy drive present, emulate the second one */
        partitions[1].drive_number = 0;
        virtual_drive = true;
    }
    /* reset flag for virtual floppy drive */
    *DISK_FLAG = 0;

    /* copy BIOS DPT and update interrupt vector */
    kfmemcpy(DOS_DPT, ivt[0x1E], sizeof(struct dpt_t));
    DOS_DPT->sectors_per_track = 36;
    ivt[0x1E] = DOS_DPT;

    for (i = 0; i < drive_count; i++) {
        kputs("Detected ");
        if (drives[i].bios_number < 0x80)
            kputs("floppy ");
        else
            kputs("hard ");
        kputs("drive ");
        kprn_ul(drives[i].bios_number);
        kputs(": ");
        if (drives[i].bios_number < 0x80) {
            kputs(floppy_drive_types[drives[i].media_type]);
            if (drives[i].type != floppy_no_detect)
                kputs(", change detection");
        } else {
            kprn_ul(drives[i].cylinder_count);
            kputs(" cylinders, ");
            kprn_ul(drives[i].head_count);
            kputs(" heads, ");
            kprn_ul(drives[i].sector_count);
            kputs(" sectors");
        }
        kputs("\r\n");
    }
}
