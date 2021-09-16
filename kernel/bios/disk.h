#ifndef __BIOS_DISK_H__
#define __BIOS_DISK_H__

int bios_load_sector(int, unsigned int, void *, int);
int bios_hdd_load_sector(int, unsigned int, void *, int);
int bios_floppy_load_sector(int, unsigned int, void *, int);
int bios_floppy_load_sector_chs(int, unsigned int, void *, int, int, int);
int bios_read_byte(int, long);
long bios_read_word(int, long);

#endif
