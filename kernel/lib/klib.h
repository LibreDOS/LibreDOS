#ifndef __KLIB_H__
#define __KLIB_H__

#define SEGMENTOF(x) ((unsigned int)((unsigned long)(x) >> 16))
#define OFFSETOF(x) ((unsigned int)((x) & 0xFFFF))
#define FARPTR(x,y) ((void far*)(((unsigned long)(x) << 16) + (y)))
#define PARA(x) (((x) & 0xF) == 0 ? (x)>>4 : ((x)>>4) + 1)

void kprn_ul(unsigned long);
void kprn_x(unsigned long);
void kputs(char *);
void kgets(char *, int);
void kpanic(char *);

void *knmemcpy(void *, void *, unsigned int);
void far *kfmemcpy(void far*, void far*, unsigned long);

void init_knalloc(void);
void init_kfalloc(void);
void *knalloc(unsigned int);
void far *kfalloc(unsigned long, unsigned int);
void kffree(void far*);
void far *kfrealloc(void far*, unsigned long);

typedef char symbol[];

#endif
