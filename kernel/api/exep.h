#ifndef __EXEP_H__
#define __EXEP_H__

enum disk_rw_t {
    read = 0, write = 1
};

enum disk_location_t {
    reserved = 0, fat = 1, directory = 2, data = 3
};

enum error_code_t {
    write_protect = 0, not_ready = 2, data_error = 4, seek_error = 6, sector_not_found = 8, write_fault = 10, general_error = 12
};

enum error_response_t {
    ignore = 0, retry = 1, abort = 2
};

void check_control_c(void);
enum error_response_t throw_disk_error(uint8_t, enum disk_rw_t, enum disk_location_t, enum error_code_t);
enum error_response_t throw_fat_error();

#endif
