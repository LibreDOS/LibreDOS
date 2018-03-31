#ifndef __BIOS_IO_H__
#define __BIOS_IO_H__

void bios_putchar(char);
void bios_puts(char *);
int bios_getchar(void);
void bios_gets(char *, int);

#endif
