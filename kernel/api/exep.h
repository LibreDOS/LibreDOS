#ifndef __EXEP_H__
#define __EXEP_H__

enum disk_rw_t {
    read, write
};

enum disk_location_t {
    reserved, fat, directory, data
};

enum error_code_t {
    write_protect, not_ready = 2, data_error = 4, seek_error = 6, sector_not_found = 8, write_fault = 10, general_error = 12
};

enum error_response_t {
    ignore, retry, abort
};

enum error_response_t throw_disk_error(uint8_t, enum disk_rw_t, enum disk_location_t, enum error_code_t);
enum error_response_t throw_fat_error(void);

#endif
