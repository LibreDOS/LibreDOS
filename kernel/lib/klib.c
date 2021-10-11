#include <bios/io.h>
#include <lib/klib.h>

void kprn_ul(unsigned long x) {
    int i;
    char buf[21];

    for (i = 0; i < 21; i++)
        buf[i] = 0;

    if (!x) {
        bios_putchar('0');
        return;
    }

    for (i = 19; x; i--) {
        buf[i] = (x % 10) + 0x30;
        x = x / 10;
    }

    i++;
    kputs(buf + i);

    return;
}

static char *hex_to_ascii_tab = "0123456789abcdef";

void kprn_x(unsigned long x) {
    int i;
    char buf[17];

    for (i = 0; i < 17; i++)
        buf[i] = 0;

    if (!x) {
        kputs("0x0");
        return;
    }

    for (i = 15; x; i--) {
        buf[i] = hex_to_ascii_tab[(x % 16)];
        x = x / 16;
    }

    i++;
    kputs("0x");
    kputs(buf + i);

    return;
}

void kgets(char *str, int limit) {
    int i = 0;
    int c;

    for (;;) {
        c = bios_getchar();
        switch (c) {
            case 0x08:
                if (i) {
                    i--;
                    bios_putchar(0x08);
                    bios_putchar(' ');
                    bios_putchar(0x08);
                }
                continue;
            case 0x0d:
                bios_putchar(0x0d);
                bios_putchar(0x0a);
                str[i] = 0;
                break;
            default:
                if (i == limit - 1)
                    continue;
                bios_putchar(c);
                str[i++] = (char)c;
                continue;
        }
        break;
    }

    return;
}

void kputs(char *str) {
    int i;

    for (i = 0; str[i]; i++) {
        bios_putchar(str[i]);
    }

    return;
}

void kpanic(char *str) {
    kputs(str);
    asm volatile ("cli");
    for (;;);
}

void *knmemcpy(void *dest, void *src, unsigned int count) {
    unsigned int i;
    char *destptr = dest;
    char *srcptr = src;

    for (i=0; i < count; i++)
        destptr[i] = srcptr[i];

    return dest;
}

void __far *kfmemcpy(void __far *dest, void __far *src, unsigned long count) {
    unsigned int i,j;
    char __far *destptr = dest;
    char __far *srcptr = src;

    for (i = 0; i < (count >> 16); i++) {
        j = 0;
        do {
            destptr[j] = srcptr[j];
            j++;
        } while (j);
        destptr += (unsigned long)FARPTR(256,0);
        srcptr += (unsigned long)FARPTR(256,0);
    }

    for (i=0; i < (count & 0xFFFF); i++)
        destptr[i] = srcptr[i];

    destptr = dest;
    srcptr = src;
    return dest;
}

extern symbol bss_end;
static unsigned int memory_base;
static unsigned int memory_end;
static int inited = 0;
static char memerror[] = "\r\nMemory allocation error!\r\n";

/* DOS-style MCB */
__attribute__((packed)) struct mcb_t {
    char type;
    unsigned int owner;
    unsigned int size;
    char reserved[11];
};

#define HEAP_CHUNK_SIZE (sizeof(struct mcb_t) >> 4)

void init_knalloc(void) {
    /* get first paragraph after kernel segment */
    memory_base = PARA((unsigned int)bss_end);

    /* get first paragraph after conventional memory */
    asm ("int $0x12" : "=a" (memory_end));
    memory_end <<= 6;

    return;
}

/* allocates a memory chunk inside the kernel segment during initialization */
void *knalloc(unsigned int size) {
    unsigned int i;
    char *ptr;

    /* make sure far memory manager not initialized */
    if (inited) return (void *)0;
    ptr = (char *)(memory_base<<4);
    /* make sure there is no overflow */
    if (ptr + size < ptr || size > 0xFFF0) return (void *)0;
    memory_base+=PARA(size);

    /* zero out memory */
    for (i = 0; i < size; i++)
        ptr[i] = 0;
    return (void *)ptr;
}

/* knalloc should not be called after this */
void init_kfalloc(void) {
    /* creates the first memory chunk */
    struct mcb_t __far *root_chunk;

    root_chunk = FARPTR(memory_base,0);

    root_chunk->type = 'Z';
    root_chunk->owner = 0;
    root_chunk->size = memory_end - (memory_base + HEAP_CHUNK_SIZE);

    inited=1;
    return;
}

void __far *kfalloc(unsigned long size, unsigned int owner) {
    /* search for a big enough, free heap chunk */
    struct mcb_t __far *heap_chunk = FARPTR(memory_base,0);
    struct mcb_t __far *new_chunk;
    unsigned long heap_chunk_ptr;
    char __far *area;
    unsigned int i,j;

    /* convert size into paragraphs */
    unsigned int paras = PARA(size);

    for (;;) {
        if (heap_chunk->type!='M' && heap_chunk->type!='Z')
            kpanic(memerror);

        if ((!heap_chunk->owner) && (heap_chunk->size == paras)) {
            /* simply mark heap_chunk as not free */
            heap_chunk->owner = owner;
            area = FARPTR(SEGMENTOF(heap_chunk) + HEAP_CHUNK_SIZE, 0);
            break;
        } else if ((!heap_chunk->owner) && (heap_chunk->size >= (paras + HEAP_CHUNK_SIZE))) {
            /* split off a new heap_chunk */
            new_chunk = FARPTR(SEGMENTOF(heap_chunk) + paras + HEAP_CHUNK_SIZE, 0);
            new_chunk->type = heap_chunk->type;
            new_chunk->owner = 0;
            new_chunk->size = heap_chunk->size - (paras + HEAP_CHUNK_SIZE);
            /* resize the old chunk */
            heap_chunk->type = 'M';
            heap_chunk->owner = owner;
            heap_chunk->size = paras;
            area = FARPTR(SEGMENTOF(heap_chunk) + HEAP_CHUNK_SIZE, 0);
            break;
        } else {
            if (heap_chunk->type == 'Z')
                return (void __far*)0;
            heap_chunk_ptr = SEGMENTOF(heap_chunk);
            heap_chunk_ptr += heap_chunk->size + HEAP_CHUNK_SIZE;
            if (heap_chunk_ptr >= memory_end)
                kpanic(memerror);
            heap_chunk = FARPTR(heap_chunk_ptr,0);
            continue;
        }
    }

    /* zero out memory */
    char __far *areaseg = area;
    for (i = 0; i < (size >> 16); i++) {
        j=0;
        do {
            areaseg[i]=0;
            j++;
        } while (j);
        areaseg += (unsigned long)FARPTR(256,0);
    }
    for (i = 0; i < (size & 0xFFFF); i++)
        areaseg[i] = 0;

    return area;
}

void kffree(void __far *addr) {
    unsigned int heap_chunk_ptr = SEGMENTOF(addr);
    __far struct mcb_t *heap_chunk, *next_chunk, *prev_chunk;

    heap_chunk_ptr -= HEAP_CHUNK_SIZE;
    heap_chunk = FARPTR(heap_chunk_ptr,0);

    heap_chunk_ptr += heap_chunk->size + HEAP_CHUNK_SIZE;
    next_chunk = FARPTR(heap_chunk_ptr,0);

    /* abort if MCB isn't valid */
    if (heap_chunk->type != 'M' && heap_chunk->type != 'Z') return;

    prev_chunk = FARPTR(memory_base,0);
    /* walk through memory blocks, since list ist only singly linked :( */
    if (SEGMENTOF(heap_chunk) != memory_base) {
        while (SEGMENTOF(prev_chunk) + prev_chunk->size + HEAP_CHUNK_SIZE != SEGMENTOF(heap_chunk)) {
            /* abort if MCB isn't in chain */
            if (prev_chunk->type == 'Z') return;
            /* panic if chain is corrupted */
            if (prev_chunk->type != 'M' || SEGMENTOF(prev_chunk) > memory_end)
                kpanic(memerror);
            /* get our next element */
            prev_chunk = FARPTR(SEGMENTOF(prev_chunk) + prev_chunk->size + HEAP_CHUNK_SIZE, 0);
        }
    }

    /* flag chunk as free */
    heap_chunk->owner = 0;

    /* if the next chunk is free as well, fuse the chunks into a single one */
    if (heap_chunk->type != 'Z' && !next_chunk->owner) {
        heap_chunk->size += next_chunk->size + HEAP_CHUNK_SIZE;
    }

    /* if the previous chunk is free as well, fuse the chunks into a single one */
    if (SEGMENTOF(heap_chunk) != memory_base && !prev_chunk->owner) {       /* if its not the first chunk */
        prev_chunk->size += heap_chunk->size + HEAP_CHUNK_SIZE;
    }

    return;
}

void __far *kfrealloc(void __far *addr, unsigned long new_size) {
    unsigned int heap_chunk_ptr = SEGMENTOF(addr);
    struct mcb_t __far *heap_chunk;
    char __far *new_ptr;

    if (!addr)
        return kfalloc(new_size,8); /* TODO: replace with current PSP? */

    if (!new_size) {
        kffree(addr);
        return (void __far*)0;
    }

    heap_chunk_ptr -= HEAP_CHUNK_SIZE;
    heap_chunk = FARPTR(heap_chunk_ptr,0);

    if ((new_ptr = kfalloc(new_size,heap_chunk->owner)) == 0)
        return (void __far*)0;

    /* convert size to paragraphs */
    unsigned int paras = PARA(new_size);

    if (heap_chunk->size > paras)
        kfmemcpy(new_ptr, addr, paras << 4);
    else
        kfmemcpy(new_ptr, addr, heap_chunk->size << 4);

    kffree(addr);

    return new_ptr;
}
