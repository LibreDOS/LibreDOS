#include <stdbool.h>
#include <stddef.h>
#include <ptrdef.h>
#include <api/exep.h>
#include <api/chario.h>
#include <lib/klib.h>
#include <bios/io.h>
#include <bios/disk.h>

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

static int drive_count = 0;
static bool virtual_drive = false;

static struct partition_t {
    bool valid;
    int drive_number;
    uint32_t start;
    uint32_t size;
} partitions[MAX_DRIVE_COUNT];

static int partition_count = 0;

static const char floppy_drive_types[6][8] = {{"unknown"},{"360k"},{"1220k"},
                                              {"720k"},{"1440k"},{"2880k"}};


static volatile union {
    unsigned char bytes[SECTOR_SIZE];
    struct __attribute__((packed)) {
        unsigned char code_area[0x1BE];
        struct mbr_entry_t partition_table[4];
        uint16_t signature;
    } mbr;
    struct __attribute__((packed)) {
        unsigned char preamble[0x0B];
        struct bpb30_t bpb;
    } vbr;
} buffer;

static int drive_streak = -1;

static unsigned int read_write_sectors(unsigned int operation, struct drive_t *drive,
                                       unsigned long start, unsigned long count,
                                       volatile void far *target) {
    uintfar_t physical_address = ((uintfar_t)SEGMENTOF(target) << 4) + OFFSETOF(target);
    unsigned int cylinder = start / (drive->head_count * drive->sector_count);
    unsigned int sector = start % (drive->head_count * drive->sector_count);
    unsigned int head = sector / drive->sector_count;
    sector %= drive->sector_count;

    if (virtual_drive && drive->bios_number == drives[0].bios_number) {
        if (*DISK_FLAG != drive - drives) {
            kputs("\r\nInsert disk for drive ");
            kputchar('A' + (drive - drives));
            kputs(": and press any key when ready\r\n");
            bios_flush();
            bios_getchar();
            *DISK_FLAG = drive - drives;
        }
    }

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
            segment = physical_address >> 4;
            offset = physical_address & 0xF;
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
                          "popw %%bx"
                          : "=a" (status),
                            "=b" (flags)
                          : "a" ((operation << 8) + transfer_size),
                            "b" (offset),
                            "c" (((cylinder & 0xFF) << 8) + (cylinder & ~0xFF >> 2) + sector + 1),
                            "d" ((head << 8) + drive->bios_number),
                            "S" (segment)
                          : "%es", "cc", "memory");
            asm volatile ("int $0x13" :: "a" (0), "d" (drive->bios_number) : "cc");
            if (status >> 8 == 0 && (flags & 1))
                status = 0x0100;
            else if (!(flags & 1))
                break;
        }
        if (flags & 1)
            return status >> 8;
        if (((uintfar_t)segment << 4) + offset != physical_address)
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

static bool convert_error (unsigned int bios_error, uint16_t *error_code, struct partition_t *partition) {
    /* conversion table to convert int 13h error codes to ones that can be returned to DOS */
    static const uint16_t extra_errors[] = {0x05, 0x06, 0x07, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
                                            0x0F, 0x11, 0x32, 0xAA, 0xBB, 0xCC, 0xE0, 0xFF};
    static const uint16_t equiv_errors[] = {0x20, 0x80, 0x20, 0x04, 0x04, 0x04, 0x04, 0x02,
                                            0x08, 0x10, 0x80, 0x80, 0x04, 0x00, 0x20, 0x40};
    /* conversion table to convert int 13h error code that is returned in AH to DOS error code */
    static const uint16_t ah_errors[] = {0x00, 0x02, 0x03, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
    static const enum error_code_t al_errors[] = {write_fault, general_error, write_protect, sector_not_found, data_error,
                                                  data_error, general_error, seek_error, not_ready};
    unsigned int i;
    if (bios_error == 0) {
        return true;
    } else {
        /* if disk change was involved, invalidate */
        if (bios_error == 6 && drives[partition->drive_number].bios_number < 0x80)
            partition->valid = false;
        /* convert int 13h error code */
        for (i = 0; i < sizeof(extra_errors)/sizeof(extra_errors[0]); i++)
            if (bios_error == extra_errors[i]) {
                bios_error = equiv_errors[i];
                break;
            }
        /* remaining error codes shouldn't happen */
        *error_code = (bios_error << 8) + general_error;
        /* find corresponding DOS error */
        for (i = 0; i < sizeof(ah_errors)/sizeof(ah_errors[0]); i++)
            if (ah_errors[i] >> 8 == bios_error)
                *error_code = (bios_error << 8) + al_errors[i];
        return false;
    }
}

static bool check_drive_number(int drive_number, uint16_t *error_code, struct partition_t **partition, struct drive_t **drive) {
    if (drive_number >= partition_count || drive_number < 0)
        return convert_error(0x80,error_code,*partition);
    *partition = &partitions[drive_number];
    if ((*partition)->drive_number < 0)
        return convert_error(0x80,error_code,*partition);
    *drive = &drives[(*partition)->drive_number];
    return true;
}

bool bios_disk_read(int drive_number, uint16_t *error_code, uint16_t start, uint16_t count, void far *dta) {
    struct partition_t *partition;
    struct drive_t *drive;
    if (!check_drive_number(drive_number,error_code,&partition,&drive))
        return false;
    if (partition->drive_number != drive_streak)
        drive_streak = -1;
    if (!partition->valid)
        return convert_error(0x80,error_code,partition);
    return convert_error(read_write_sectors(2,drive,partition->start + start,count,dta),error_code,partition);
}

bool bios_disk_write(int drive_number, uint16_t *error_code, uint16_t start, uint16_t count, void far *dta) {
    struct partition_t *partition;
    struct drive_t *drive;
    if (!check_drive_number(drive_number,error_code,&partition,&drive))
        return false;
    if (partition->drive_number != drive_streak)
        drive_streak = -1;
    if (!partition->valid)
        return convert_error(0x80,error_code,partition);
    return convert_error(read_write_sectors(3,drive,partition->start + start,count,dta),error_code,partition);
}

bool bios_disk_change(int drive_number, union disk_change_t *result) {
    struct partition_t *partition;
    struct drive_t *drive;
    if (!check_drive_number(drive_number,&result->error_code,&partition,&drive))
        return false;

    if (!partition->valid)
        result->change_status = changed;
    else if (drive->bios_number >= 0x80 || drive->type == hard_drive)
        result->change_status = didnt_change;
    else if (drive->type == floppy_no_detect || (virtual_drive && drive->bios_number == drives[0].bios_number)) {
        result->change_status = dont_know;
    } else {
        uint16_t flags;
        asm volatile ("int $0x13\n"
                      "pushf\n"
                      "popw %%ax" : "=a" (flags) : "a" (0x1600), "d" (drive->bios_number), "S" (0));
        if (flags & 1) {
            asm volatile ("int $0x13" :: "a" (0), "d" (drive->bios_number) : "cc");
            result->change_status = changed;
            partition->valid = false;
        } else {
            /* check if another drive was accessed since last check */
            if (drive_streak == partition->drive_number) {
                result->change_status = didnt_change;
            } else {
                result->change_status = dont_know;
            }
        }
        drive_streak = partition->drive_number;
    }
    return true;
}

bool bios_disk_build_bpb(int drive_number, uint16_t *error_code, struct bpb_t *bpb) {
    struct partition_t *partition;
    struct drive_t *drive;
    bool valid_bpb = true, valid_bpb30 = true;
    if (!check_drive_number(drive_number,error_code,&partition,&drive))
        return false;
    if (partition->drive_number != drive_streak)
        drive_streak = -1;
    if (!partition->valid)
        drive->head_count = drive->sector_count = 2;

    /* load boot sector */
    if (!convert_error(read_write_sectors(2,drive,partition->start,1,buffer.bytes),error_code,partition))
        return false;
    knmemcpy(bpb,(void *)&buffer.vbr.bpb.bpb,sizeof(struct bpb_t));
    if (!partition->valid) {
        drive->sector_count = buffer.vbr.bpb.sector_count;
        drive->head_count = buffer.vbr.bpb.head_count;
    } else if (drive->bios_number < 0x80)
        partition->valid = false;
    /* check boot sector BPB values */
    if (bpb->bytes_per_sector != SECTOR_SIZE)
        valid_bpb = false;
    else if (bpb->sectors_per_cluster & (bpb->sectors_per_cluster - 1))
        valid_bpb = false; /* check if sectors per cluster is a power of two */
    else if (bpb->sectors_per_cluster == 0)
        valid_bpb = false;
    else if (bpb->reserved_sectors == 0)
        valid_bpb = false;
    else if (bpb->fat_count == 0)
        valid_bpb = false;
    else if (bpb->root_entries == 0)
        valid_bpb = false;
    else if (bpb->root_entries & 0x0F)
        valid_bpb = false;
    else if (bpb->sector_count <= (bpb->root_entries >> 4) + bpb->sectors_per_fat * bpb->fat_count + bpb->reserved_sectors)
        valid_bpb = false;
    else if (bpb->media_descriptor < 0xF0)
        valid_bpb = false;
    /* check for DOS 3.0 BPB */
    if (drive->sector_count == 0 || drive->sector_count > 63) {
        valid_bpb30 = false;
        drive->head_count = drive->sector_count = 2;
    } else if (drive->head_count == 0 || drive->head_count > 256) {
        valid_bpb30 = false;
        drive->head_count = drive->sector_count = 2;
    }

    /* load first FAT to compare media descriptor */
    if (valid_bpb) {
        if (read_write_sectors(2,drive,partition->start + bpb->reserved_sectors,1,buffer.bytes) != 0)
            valid_bpb = false;
        else if (bpb->media_descriptor != buffer.bytes[0])
            valid_bpb = false;
    }
    /* require hard drives to have a valid DOS 2.0 BPB and have media descriptor F8 */
    if (drive->bios_number >= 0x80 && (!valid_bpb || bpb->media_descriptor != 0xF8))
        return convert_error(0x80,error_code,partition);
    else if (!valid_bpb) {
        if (!convert_error(read_write_sectors(2,drive,partition->start + 1,1,buffer.bytes),error_code,partition))
            return false;
        /* absence of BPB realistically implies double density 8 sector 5.25" floppy disks */
        if ((bpb->media_descriptor = buffer.bytes[0]) < 0xFE)
            return convert_error(0x80,error_code,partition);
    }

    /* check if it is a double density 5.25" floppy disk with possibly no BPB */
    if (bpb->media_descriptor >= 0xFC) {
        drive->sector_count = 9 - (bpb->media_descriptor >> 1 & 1);
        drive->head_count = 1 + (bpb->media_descriptor & 1);
        bpb->bytes_per_sector = SECTOR_SIZE;
        bpb->sectors_per_cluster = 1 + (bpb->media_descriptor & 1);
        bpb->reserved_sectors = 1;
        bpb->fat_count = 2;
        bpb->root_entries = 64 + 48 * (bpb->media_descriptor & 1);
        bpb->sector_count = drive->sector_count * drive->head_count * 40;
        bpb->sectors_per_fat = 2 - (bpb->media_descriptor >> 1 & 1);
    } else if (drive->bios_number < 0x80 && (!valid_bpb30 || !valid_bpb))
        return convert_error(0x80,error_code,partition);
    partition->valid = true;
    return true;
}


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

int bios_disk_init(void) {
    /* get the amount of floppy drives */
    uint16_t equipment = 0;
    int floppy_count = 0, i;
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
            drives[i].media_type = unknown;
        }
        drive_count = floppy_count;
    }
    /* if only one floppy drive present, emulate the second one */
    if (drive_count == 1) {
        drive_count = 2;
        drives[1].bios_number = drives[0].bios_number;
        drives[1].type = drives[0].type;
        drives[1].media_type = drives[0].media_type;
        virtual_drive = true;
    }

    /* initialize floppy partitions */
    partition_count = drive_count;
    for (i = 0; i < drive_count; i++)
        partitions[i].drive_number = i;
    /* reserve drive letters A: and B: */
    if (partition_count < 2) {
        partition_count = 2;
        partitions[0].drive_number = -1;
        partitions[1].drive_number = -1;
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
            if (entry->lba_start != 0 && entry->lba_size != 0) {
                partition = &partitions[partition_count++];
                partition->valid = true;
                partition->drive_number = i;
                partition->start = entry->lba_start;
                partition->size = entry->lba_size;
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

    return partition_count;
}
