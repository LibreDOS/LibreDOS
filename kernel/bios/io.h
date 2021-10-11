#ifndef __BIOS_IO_H__
#define __BIOS_IO_H__

void bios_init(void);

void bios_putchar(char);
char bios_getchar(void);
unsigned int bios_status();
void bios_com_putchar(unsigned int, char);
char bios_com_getchar(unsigned int);
void bios_lpt_putchar(unsigned int, char);

#endif
