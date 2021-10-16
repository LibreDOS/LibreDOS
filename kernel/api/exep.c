#include<stdint.h>
#include<ptrdef.h>
#include<bios/io.h>
#include<api/exep.h>

extern void int23_dispatch(void);
extern enum error_response_t int24_dispatch(uint16_t, uint16_t);

void check_control_c(void) {
    /* check if ctrl+c in keyboard buffer */
    if ((bios_status() & 0xFF) == 3) {
        /* remove from buffer */
        bios_getchar();
        /* let the int 23h handler do its stuff */
        int23_dispatch();
    }
}

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
