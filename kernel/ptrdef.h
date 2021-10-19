#ifndef __PTRDEF_H__
#define __PTRDEF_H__

#define far __far
#define SEGMENTOF(x) ((unsigned int)((unsigned long)(x) >> 16))
#define OFFSETOF(x) ((unsigned int)((x) & 0xFFFF))
#define FARPTR(x,y) ((void far *)(((unsigned long)(x) << 16) + (y)))
#define PARA(x) (((x) & 0xF) == 0 ? (x)>>4 : ((x)>>4) + 1)

extern void far ** const ivt;
extern void * const nullptr;

#endif
