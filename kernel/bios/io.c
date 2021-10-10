#include <stdint.h>
#include <bios/io.h>

static int com_ports;
static int lpt_ports;
static char key_code = 0;

void bios_init(void) {
    uint16_t equiment = 0;
    asm ("int $0x11" : "=a" (equipment));
    com_ports = (equipment >> 9) & 7;
    lpt_ports = (equipment >> 14) & 3;
}

void bios_putchar(char c) {
    asm volatile ("int $0x10" :: "a" (0x0e00 | c), "b" (0) : "%bp");
}

char bios_getchar(void) {
    uint16_t ret = 0;
    if (key_code) {
        ret = key_code;
        key_code = 0;
        return ret;
    }

    asm volatile ("int $0x16" : "=a" (ret));
    if (!ret) key_code = ret >> 8;
    return ret & 0xff;
}

input_status_t bios_status(void) {
    input_status_t status;

    if (key_code) {
        status.available = 1;
        status.character = key_code;
        return status;
    }

    asm volatile ("int $0x16\n"
                  "pushf"
                  "pop bx" : "=a" (1) : "a" (status.character), "b" (status.available));
    /* extract zero flag */
    status.available &= 0x40;
    return status;
}
