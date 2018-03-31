#include <bios_io.h>

void kmain(void) {
    bios_puts("Welcome to LibreDOS!");

    for (;;)
        bios_getchar();
}
