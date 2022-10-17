#ifndef  __ALLOC_H__
#define __ALLOC_H__

void init_knalloc(void);
void *knalloc(size_t);

#ifdef VERSION_2_0
void init_kfalloc(void);
void far *kfalloc(farsize_t, unsigned int);
void kffree(void far *);
void far *kfrealloc(void far *, farsize_t);
#endif

typedef char symbol[];

#endif
