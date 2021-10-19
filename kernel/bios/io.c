#include <stdint.h>
#include <ptrdef.h>
#include <bios/io.h>

static unsigned int com_ports;
static unsigned int lpt_ports;
volatile char key_code = 0;
extern void int1B(void);

void bios_init(void) {
    uint16_t equipment = 0;
    unsigned int i;
    asm ("int $0x11" : "=a" (equipment));
    com_ports = (equipment >> 9) & 7;
    lpt_ports = (equipment >> 14) & 3;
    /* intialize every serial port to 2400 baud 8N1 */
    for (i=0; i < com_ports; i++)
        asm volatile ("int $0x14" :: "a" (0x00A3), "d" (i));
    for (i=0; i < lpt_ports; i++)
        asm volatile ("int $0x17" :: "a" (0x0100), "d" (i));
    /* install interrupt 1Bh handler */
    asm volatile ("cli");
    ivt[0x1B] = int1B;
    asm volatile ("sti");
}

void bios_putchar(char c) {
    asm volatile ("int $0x10" :: "a" (0x0e00 | c), "b" (0) : "%bp");
}

char bios_getchar(void) {
    uint16_t ret = 0;

    /* repeat until we get a key */
    while (!ret) {
        /* check if he have scancode stored or if int 1Bh gave us one */
        if (key_code) {
            asm volatile ("cli");
            ret = key_code;
            key_code = 0;
            asm volatile ("sti");
            return ret;
        }
        /* ask BIOS for key */
        asm volatile ("int $0x16" : "=a" (ret) : "a" (0));
    }

    char character = ret & 0xFF;
    /* store key */
    if (!character) key_code = ret >> 8;
    return ret & 0xff;
}

unsigned int bios_status(void) {
    uint16_t ret, flags;

    if (key_code)
        return key_code;

    asm volatile ("int $0x16\n"
                  "pushf\n"
                  "popw %%bx" : "=a" (ret), "=b" (flags) : "a" (0x0100));
    /* check zero flag */
    if (flags & 0x0040)
        return 0;
    else
        return ret;
}

void bios_flush(void) {
    while (bios_status())
        bios_getchar();
}

void bios_com_putchar(unsigned int port, char c) {
    if (port >= com_ports) return;
    asm volatile ("int $0x14" :: "a" (0x0100 + c), "d" (port));
}

char bios_com_getchar(unsigned int port) {
    /* no critical error on error, since that would require DOS 2.0+ device drivers
       DOS 1.0 just ignores character device errors, we return 0xFF */
    if (port >= com_ports) return 0xFF;
    uint16_t ret = 0;
    asm volatile ("int $0x14" : "=a" (ret) : "a" (0x0200), "d" (port));
    if (ret & 0x8E00) return 0xFF;
    else return ret & 0xFF;
}

void bios_lpt_putchar(unsigned int port, char c) {
    if (port >= lpt_ports) return;
    asm volatile ("int $0x17" :: "a" (c), "d" (port));
}
