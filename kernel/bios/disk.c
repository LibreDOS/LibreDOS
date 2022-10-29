#include <stdbool.h>
#include <stddef.h>
#include <ptrdef.h>
#include <bios/disk.h>
#include <api/exep.h>
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

/* MBR partition table entry */
__attribute__((packed)) struct mbr_entry_t {
    uint8_t active;
    uint8_t start_head;
    uint16_t start_cylinder_sector;
    uint8_t partition_type;
    uint8_t end_head;
    uint16_t end_cylinder_sector;
    uint32_t lba_start;
    uint32_t lba_size;
};


#define MAX_DRIVE_COUNT ('Z'-'A'+1)
#define SECTOR_SIZE 512

static struct drive_t {
    unsigned int bios_number;
    enum {
        none, floppy_no_detect, floppy_swap_detect, hard_drive
    } type;
    enum {
        unknown, floppy_360, floppy_1220, floppy_720, floppy_1440, floppy_2880
    } media_type;
    unsigned int cylinder_count, head_count, sector_count;
} drives[MAX_DRIVE_COUNT];

static unsigned int drive_count = 0;
static bool virtual_drive = false;

static struct partition_t {
    bool valid;
    int drive_number;
    uint32_t start;
    uint32_t size;
} partitions[MAX_DRIVE_COUNT];

static unsigned int partition_count = 0;

static const char floppy_drive_types[6][8] = {{"unknown"},{"360k"},{"1220k"},
                                              {"720k"},{"1440k"},{"2880k"}};


struct int13_8_t {
    uint16_t flags, floppy_type, cylinder_sector, head_drive;
};

/* read info from function 8 */
static bool int13_8 (unsigned int drive, struct int13_8_t *data) {
    asm("int $0x13\n"
        "pushf\n"
        "popw %%ax\n"
        "sti"
        : "=a" (data->flags), "=b" (data->floppy_type),
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
static bool int13_15 (unsigned int drive, struct int13_15_t *data) {
    asm("int $0x13\n"
        "pushf\n"
        "popw %%bx"
        : "=a" (data->drive_type), "=b" (data->flags)
        : "a" (0x15FF), "d" (drive) : "%cx", "cc");
    asm volatile ("int $0x13" :: "a" (0x0100), "d" (drive) : "cc");
    /* guard against bug in AX return value */
    if (data->drive_type == 3)
        data->drive_type <<= 8;
    data->drive_type >>= 8;
    return !(data->flags & 1) && data->drive_type > 0 && data->drive_type < 4;
}


volatile union {
    unsigned char bytes[SECTOR_SIZE];
    __attribute__((packed)) struct {
        unsigned char code_area[0x1BE];
        struct mbr_entry_t partition_table[4];
        uint16_t signature;
    } mbr;
} buffer;

static unsigned int read_write_sectors(unsigned int operation, struct drive_t *drive,
                                       unsigned long start, unsigned long count,
                                       volatile void far *target) {
    uintfar_t physical_address = (SEGMENTOF(target) << 4) + OFFSETOF(target);
    unsigned int cylinder = start / (drive->head_count * drive->sector_count);
    unsigned int sector = start % (drive->head_count * drive->sector_count);
    unsigned int head = sector / drive->sector_count;
    sector %= drive->sector_count;

    while (count) {
        unsigned int transfer_size = count;
        segment_t segment;
        uintptr_t offset;
        unsigned int i;
        uint16_t status, flags;
        /* check if transfer would go over 64k boundary */
        if (((physical_address + (count << 9)) ^ physical_address) & ~0xFFFFul)
            transfer_size = ((physical_address & ~0xFFFFul) + 0x10000ul - physical_address) >> 9;
        /* check if transfer would go over track boundary */
        if (transfer_size > drive->sector_count - sector)
            transfer_size = drive->sector_count - sector;

        /* when going across a 64k boundary,
         * or if it is on a buffer with a odd physical address and it's the last sector,
         * use the char array as a buffer for that sector */
        if (transfer_size && (!(physical_address & 1) || count > 1)) {
            segment = physical_address >> 16;
            offset = physical_address & 0xFFFF;
        } else {
            transfer_size = 1;
            segment = 0;
            offset = (uintptr_t)buffer.bytes;
        }
        for (i = 0; i <= 3; i++) {
            asm volatile ("movw %%si, %%es\n"
                          "stc\n"
                          "int $0x13\n"
                          "pushf\n"
                          "popw %%bx\n"
                          : "=a" (status),
                            "=b" (flags)
                          : "a" ((operation << 8) + transfer_size),
                            "b" (offset),
                            "c" (((cylinder & 0xFF) << 8) + (cylinder & ~0xFF >> 2) + sector + 1),
                            "d" ((head << 8) + drive->bios_number),
                            "S" (segment)
                          : "%es", "cc", "memory");
            asm volatile ("int $0x13" :: "a" (0), "d" (drive->bios_number) : "cc");
            if (status == transfer_size)
                break;
        }
        if (status != transfer_size)
            return status >> 8;
        if (segment == 0 && offset == (uintptr_t)buffer.bytes)
            kfmemcpy((void far *)physical_address, FARPTR(0,buffer.bytes), SECTOR_SIZE);

        count -= transfer_size;
        physical_address += transfer_size << 9;
        sector += transfer_size;
        if (sector >= drive->sector_count) {
            sector = 0;
            head++;
            if (head >= drive->head_count) {
                head = 0;
                cylinder++;
            }
        }
    }
    return 0;
}


static void scan_drives(unsigned int start, unsigned int end, unsigned int count) {
    unsigned int detected_count = 0, i;
    bool use_int13_15 = false;
    struct int13_8_t int13_8_data;
    struct int13_15_t int13_15_data;

    /* get drive count from function 8, if zero was passed */
    for (i = start; i < end && count == 0; i++) {
        if (int13_8(i,&int13_8_data))
            count = int13_8_data.head_drive & 0xFF;
    }
    if (drive_count + count > MAX_DRIVE_COUNT)
        count = MAX_DRIVE_COUNT - drive_count;

    /* scan drive numbers for drives */
    for (i = start; i < end && (detected_count < count || !use_int13_15); i++) {
        bool int13_8_valid, int13_15_valid;
        struct drive_t drive;
        drive.bios_number = i;
        drive.type = floppy_no_detect;
        if ((int13_8_valid = int13_8(i,&int13_8_data))) {
            /* convert floppy media type return value */
            if (int13_8_data.floppy_type == 6)
                int13_8_data.floppy_type--;
            if (int13_8_data.floppy_type < 6)
                drive.media_type = int13_8_data.floppy_type;
            /* get disk geometry */
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
         * works on function 15 (i.e. presumably not implemented)
         * for hard drives, a valid function 8 return value is required */
        if (detected_count < count
            && (((!use_int13_15 || int13_15_valid) && int13_8_valid)
                || (i < 0x80 && int13_15_valid)))
            drives[drive_count + detected_count++] = drive;
    }

    drive_count += detected_count;
}

void bios_disk_init(void) {
    /* get the amount of floppy drives */
    uint16_t equipment = 0;
    unsigned int floppy_count = 0, i;
    struct drive_t *drive;
    struct partition_t *partition;

    /* try int 11h first */
    asm ("int $0x11" : "=a" (equipment));
    if (equipment & 1)
        floppy_count = ((equipment >> 6) & 3) + 1;
    /* scan for floppy drives */
    scan_drives(0,0x80,floppy_count);
    /* if nothing found, assume those functions weren't implemented yet */
    if (drive_count == 0) {
        for (i = 0; i < floppy_count; i++) {
            drives[i].bios_number = i;
            drives[i].type = floppy_no_detect;
        }
        drive_count = floppy_count;
    }

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

    /* detect hard drives */
    if (*HDD_COUNT != 0)
        scan_drives(0x80,0x100,*HDD_COUNT);

    /* initialize hard drive partitions */
    for (i = floppy_count; i < drive_count && partition_count < MAX_DRIVE_COUNT; i++) {
        volatile struct mbr_entry_t *entry;
        drive = &drives[i];
        /* skip drive, if we got questionable geometry */
        if (drive->cylinder_count == 0 || drive->head_count == 0 || drive->sector_count == 0)
            continue;
        /* read in MBR and see what it has to offer */
        if (read_write_sectors(2,drive,0,1,buffer.bytes))
            continue;
        if (buffer.mbr.signature != 0xAA55)
            continue;

        for (entry = buffer.mbr.partition_table; entry < buffer.mbr.partition_table + 4; entry++) {
            if (entry->active & 0x7F)
                continue;
            /* convert start CHS to LBA */
            unsigned int sector = entry->start_cylinder_sector;
            unsigned int cylinder = (sector >> 8) + ((sector & 0xC0) << 2);
            sector = (sector & 0x3F) - 1;
            unsigned long start_chs = drive-> head_count * drive->sector_count * (unsigned long)cylinder;
            start_chs += drive->sector_count * entry->start_head + sector;
            /* convert end CHS to LBA */
            sector = entry->end_cylinder_sector;
            cylinder = (sector >> 8) + ((sector & 0xC0) << 2);
            sector = (sector & 0x3F) - 1;
            unsigned long end_chs = drive-> head_count * drive->sector_count * (unsigned long)cylinder;
            end_chs += drive->sector_count * entry->end_head + sector;

            /* if LBA fields are not zero use that, otherwise use CHS */
            if (entry->lba_start != 0 && entry->lba_size != 0) {
                partition = &partitions[partition_count++];
                partition->valid = true;
                partition->drive_number = i;
                partition->start = entry->lba_start;
                partition->size = entry->lba_size;
            } else if (start_chs != 0 && end_chs != 0 && end_chs > start_chs) {
                partition = &partitions[partition_count++];
                partition->valid = true;
                partition->drive_number = i;
                partition->start = start_chs;
                partition->size = end_chs - start_chs + 1;
            }
        }
    }

    /* print out results */
    for (drive = drives; drive < drives + drive_count; drive++) {
        kputs("Detected ");
        if (drive->bios_number < 0x80)
            kputs("floppy ");
        else
            kputs("hard ");
        kputs("drive ");
        kprn_ul(drive->bios_number);
        kputs(": ");
        if (drive->bios_number < 0x80) {
            kputs(floppy_drive_types[drive->media_type]);
            if (drive->type != floppy_no_detect)
                kputs(", change detection");
        } else {
            kprn_ul(drive->cylinder_count);
            kputs(" cylinders, ");
            kprn_ul(drive->head_count);
            kputs(" heads, ");
            kprn_ul(drive->sector_count);
            kputs(" sectors");
        }
        kputs("\r\n");
    }

    for (i = 0; i < partition_count; i++) {
        partition = &partitions[i];
        kputs("Assigned ");
        kputs("drive letter ");
        kputchar('A' + i);
        if (partition->drive_number >= 0) {
            kputs(": to drive ");
            kprn_ul(drives[partition->drive_number].bios_number);
        } else
            kputs(": to nothing");
        if (partition->valid) {
            kputs(" (start: ");
            kprn_ul(partition->start);
            kputs(", length: ");
            kprn_ul(partition->size);
            kputchar(')');
        }
        kputs("\r\n");
    }
}
