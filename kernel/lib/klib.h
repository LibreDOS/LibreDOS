#ifndef __KLIB_H__
#define __KLIB_H__

void kprn_ul(unsigned long);
void kprn_x(unsigned long);
void init_kalloc(void);
void *kalloc(unsigned int);
void kfree(void *);
void *krealloc(void *, unsigned int);

typedef char symbol[];

#endif
