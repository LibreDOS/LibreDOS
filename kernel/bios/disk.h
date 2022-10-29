#ifndef __BIOS_DISK_H__
#define __BIOS_DISK_H__

struct disk_change_t {
    bool successful;
    union {
        enum {
            dont_know, didnt_change, changed = -1
        } change_status;
        enum error_code_t error_code;
    } result;
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
}

int bios_disk_init(void);
uint16_t bios_disk_read(int drive_number, uint16_t start, uint16_t count, void far *dta);
uint16_t bios_disk_write(int drive_number, uint16_t start, uint16_t count, void far *dta);
struct disk_change_t bios_disk_change(int drive_number);
uint16_t bios_disk_build_bpb(int drive_number, struct bpb_t *bpb);

#endif
