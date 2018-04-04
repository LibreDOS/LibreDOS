#include <bios_disk.h>
#include <system.h>

#define     FLOPPY_HPC     2
#define     FLOPPY_SPT     18

int bios_floppy_load_sector(int drive, unsigned int seg,
                            void *buf, int sector) {
    /* convert LBA to CHS and call the CHS function */

    int cyl, head, sect;

    cyl = sector / (FLOPPY_HPC * FLOPPY_SPT);
    head = (sector / FLOPPY_SPT) % FLOPPY_HPC;
    sect = (sector % FLOPPY_SPT) + 1;

    return bios_floppy_load_sector_chs(drive, seg, buf, cyl, head, sect);
}

int bios_load_sector(int drive, unsigned int seg,
                     void *buf, int sector) {

    if (drive < 0x80)
        return bios_floppy_load_sector(drive, seg, buf, sector);
    else
        return bios_hdd_load_sector(drive, seg, buf, sector);

}

static char *drive_cache = 0x0000;
static int cached_drive;
static int cached_sect;
static int cache_status = 0;

int bios_read_byte(int drive, long loc) {
    int ret, old_ds;

    if (cached_drive == drive
     && cached_sect == (loc / 512)
     && cache_status) {
        old_ds = xchg_ds(0x9000);
        ret = drive_cache[loc % 512];
        xchg_ds(old_ds);
        return ret;
    } else {
        if (bios_load_sector(drive, 0x9000, drive_cache, loc / 512)) {
            cache_status = 0;
            return -1;
        } else {
            cached_drive = drive;
            cached_sect = loc / 512;
            cache_status = 1;
            old_ds = xchg_ds(0x9000);
            ret = drive_cache[loc % 512];
            xchg_ds(old_ds);
            return ret;
        }
    }

}
