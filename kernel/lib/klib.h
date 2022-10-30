#ifndef __KLIB_H__
#define __KLIB_H__

void kprn_ul(unsigned long);
void kprn_x(unsigned long);
uint16_t kread_hex(const char *, unsigned int *);
void kpanic(char *);

void *knmemcpy(void *, const void *, unsigned int);
void far *kfmemcpy(void far *, const void far *, unsigned long);

#endif
