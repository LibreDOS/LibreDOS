#ifndef __BIOS_IO_H__
#define __BIOS_IO_H__

void bios_init(void);

struct input_status_t {
    unsigned int available;
    char character;
};

void bios_putchar(char);
char bios_getchar(void);
input_status_t bios_status();
char bios_com_getchar(unsigned int, void);
void bios_com_putchar(unsigned int, char);
void bios_lpt_putchar(unsigned int, char);

#endif
