#ifndef __PTRDEF_H__
#define __PTRDEF_H__

#define far __far
#define SEGMENTOF(x) ((segment_t)((uintfar_t)(x) >> 16))
#define OFFSETOF(x) ((uintptr_t)((x) & 0xFFFF))
#define FARPTR(x,y) ((void far *)(((uintfar_t)(x) << 16) + (y)))
#define PARA(x) (((x) & 0xF) == 0 ? (x)>>4 : ((x)>>4) + 1)

typedef uint16_t segment_t;
typedef uint32_t uintfar_t;
typedef uint32_t farsize_t;

extern void far ** const ivt;
extern void * const nullptr;

#endif
