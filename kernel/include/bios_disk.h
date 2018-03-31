#ifndef __BIOS_DISK_H__
#define __BIOS_DISK_H__

int bios_floppy_load_sector_chs(int drive, unsigned int seg,
                                void *buf, int cyl, int head, int sect);

#endif
