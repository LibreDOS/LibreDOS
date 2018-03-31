#include <bios_io.h>

void kmain(void) {
    bios_puts("hello world");
	for (;;) {
		int c = bios_getchar();
		if(c < 256) {
			bios_putchar(c);
		}
	}
    for (;;);
}
