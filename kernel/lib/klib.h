#ifndef __KLIB_H__
#define __KLIB_H__

#define SEGMENTOF(x) ((unsigned int)((unsigned long)(x) >> 16))
#define OFFSETOF(x) ((unsigned int)((x) & 0xFFFF))
#define FARPTR(x,y) ((void __far*)(((unsigned long)(x) << 16) + (y)))
#define PARA(x) (((x) & 0xF) == 0 ? (x)>>4 : ((x)>>4) + 1)

void kprn_ul(unsigned long);
void kprn_x(unsigned long);
void kputs(char *);
void kgets(char *, int);
void init_kalloc(void);
void __far *kalloc(unsigned int);
void kfree(void __far*);
void __far *krealloc(void __far*, unsigned int);

typedef char symbol[];

#endif
