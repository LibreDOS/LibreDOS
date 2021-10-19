#ifndef  __ALLOC_H__
#define __ALLOC_H__

void init_knalloc(void);
void *knalloc(unsigned int);

void init_kfalloc(void);
void far *kfalloc(unsigned long, unsigned int);
void kffree(void far *);
void far *kfrealloc(void far *, unsigned long);

typedef char symbol[];

#endif
