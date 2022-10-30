#ifndef __BIOS_DISK_H__
#define __BIOS_DISK_H__

union disk_change_t {
    enum {
        dont_know, didnt_change, changed = -1
    } change_status;
    uint16_t error_code;
};

__attribute__((packed)) struct bpb_t {
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_count;
    uint16_t root_entries;
    uint16_t sector_count;
    uint8_t media_descriptor;
    uint16_t sectors_per_fat;
};

__attribute__((packed)) struct bpb30_t {
    struct bpb_t bpb;
    uint16_t sector_count;
    uint16_t head_count;
};

int bios_disk_init(void);
bool bios_disk_read(int, uint16_t *, uint16_t, uint16_t, void far *);
bool bios_disk_write(int, uint16_t *, uint16_t, uint16_t, void far *);
bool bios_disk_change(int, union disk_change_t *);
bool bios_disk_build_bpb(int, uint16_t *, struct bpb_t *);

#endif
