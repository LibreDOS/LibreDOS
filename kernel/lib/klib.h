#ifndef __KLIB_H__
#define __KLIB_H__

void kprn_ul(unsigned long);
void kprn_x(unsigned long);
void kputs(char *);
void kgets(char *, int);
void kpanic(char *);

void *knmemcpy(void *, void *, unsigned int);
void far *kfmemcpy(void far *, void far *, unsigned long);

#endif
