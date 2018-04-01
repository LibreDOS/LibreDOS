#include <bios_disk.h>

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
