#include<stdint.h>
#include<api/exep.h>

extern enum error_response_t int24_dispatch(uint16_t, uint16_t);

enum error_response_t throw_disk_error(uint8_t drive, enum disk_rw_t operation, enum disk_location_t location, enum error_code_t error_code) {
    enum error_response_t response = int24_dispatch((location << 9) + (operation << 9) + drive, error_code);
    if (response == abort)
        asm volatile ("int $0x20");
    return response;
}

enum error_response_t throw_fat_error(void) {
    enum error_response_t response = int24_dispatch(0x8000,0xFFFF);
    if (response == abort)
        asm volatile ("int $0x20");
    return response;
}
