#include <stdint.h>
#include <bios/io.h>

void bios_putchar(char c) {
    asm volatile ("int $0x10" :: "a" (0x0e00 | c), "b" (0) : "%bp");
}

int bios_getchar(void) {
    uint16_t ret = 0;
    asm volatile ("int $0x16" : "+a" (ret));
    return ret & 0xff;
}
