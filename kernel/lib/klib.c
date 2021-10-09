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

void __far *kmemcpy(void __far *dest, void __far *src, unsigned int count) {
    unsigned int i;
    char __far *destptr = dest;
    char __far *srcptr = src;

    for (i = 0; i < count; i++)
        destptr[i] = srcptr[i];

    return dest;
}

extern symbol bss_end;
static unsigned int memory_base;
static unsigned int memory_end;

struct heap_chunk_t {
    int free;
    unsigned int size;
    unsigned int prev_chunk;
    int padding[5];
};

#define HEAP_CHUNK_SIZE (sizeof(struct heap_chunk_t) >> 4)

void init_kalloc(void) {
    /* creates the first memory chunk */
    struct heap_chunk_t __far *root_chunk;

    /* get first paragraph after kernel segment */
    memory_base = PARA((unsigned int)bss_end);

    /* get first paragraph after conventional memory */
    asm ("int $0x12" : "=a" (memory_end));
    memory_end <<= 6;

    root_chunk = FARPTR(memory_base,0);

    root_chunk->free = 1;
    root_chunk->size = memory_end - (memory_base + HEAP_CHUNK_SIZE);
    root_chunk->prev_chunk = 0;

    return;
}

void __far *kalloc(unsigned int size) {
    /* search for a big enough, free heap chunk */
    struct heap_chunk_t __far *heap_chunk = FARPTR(memory_base,0);
    struct heap_chunk_t __far *new_chunk;
    struct heap_chunk_t __far *next_chunk;
    unsigned long heap_chunk_ptr;
    char __far *area;
    unsigned int i;

    /* convert size into paragraphs */
    size = PARA(size);

    for(;;) {
        if ((heap_chunk->free) && (heap_chunk->size == size)) {
            /* simply mark heap_chunk as not free */
            heap_chunk->free = !heap_chunk->free;
            area = FARPTR(heap_chunk + HEAP_CHUNK_SIZE, 0);
            break;
        } else if ((heap_chunk->free) && (heap_chunk->size >= (size + HEAP_CHUNK_SIZE))) {
            /* split off a new heap_chunk */
            new_chunk = FARPTR(SEGMENTOF(heap_chunk) + size + HEAP_CHUNK_SIZE, 0);
            new_chunk->free = 1;
            new_chunk->size = heap_chunk->size - (size + HEAP_CHUNK_SIZE);
            new_chunk->prev_chunk = SEGMENTOF(heap_chunk);
            /* resize the old chunk */
            heap_chunk->free = !heap_chunk->free;
            heap_chunk->size = size;
            /* tell the next chunk where the old chunk is now */
            next_chunk = FARPTR(SEGMENTOF(new_chunk) + new_chunk->size + HEAP_CHUNK_SIZE, 0);
            next_chunk->prev_chunk = SEGMENTOF(new_chunk);
            area = FARPTR(SEGMENTOF(heap_chunk) + HEAP_CHUNK_SIZE, 0);
            break;
        } else {
            heap_chunk_ptr = SEGMENTOF(heap_chunk);
            heap_chunk_ptr += heap_chunk->size + HEAP_CHUNK_SIZE;
            if (heap_chunk_ptr >= memory_end)
                return (void __far*)0;
            heap_chunk = FARPTR(heap_chunk_ptr,0);
            continue;
        }
    }

    /* zero out memory */
    for (i = 0; i < size; i++)
        area[i] = 0;

    return area;
}

void kfree(void __far *addr) {
    unsigned int heap_chunk_ptr = SEGMENTOF(addr);
    __far struct heap_chunk_t *heap_chunk, *next_chunk, *prev_chunk;

    heap_chunk_ptr -= HEAP_CHUNK_SIZE;
    heap_chunk = FARPTR(heap_chunk_ptr,0);

    heap_chunk_ptr += heap_chunk->size + HEAP_CHUNK_SIZE;
    next_chunk = FARPTR(heap_chunk_ptr,0);

    prev_chunk = FARPTR(heap_chunk->prev_chunk,0);

    /* flag chunk as free */
    heap_chunk->free = 1;

    /* if the next chunk is free as well, fuse the chunks into a single one */
    if (SEGMENTOF(next_chunk) >= memory_end && next_chunk->free) {
        heap_chunk->size += next_chunk->size + HEAP_CHUNK_SIZE;
        /* update next chunk ptr */
        next_chunk = FARPTR(SEGMENTOF(next_chunk) + next_chunk->size + HEAP_CHUNK_SIZE, 0);
        /* update new next chunk's prev to ourselves */
        next_chunk->prev_chunk = SEGMENTOF(heap_chunk);
    }

    /* if the previous chunk is free as well, fuse the chunks into a single one */
    if (prev_chunk) {       /* if its not the first chunk */
        if (prev_chunk->free) {
            prev_chunk->size += heap_chunk->size + HEAP_CHUNK_SIZE;
            /* notify the next chunk of the change */
            if (SEGMENTOF(next_chunk) < memory_end)
                next_chunk->prev_chunk = SEGMENTOF(prev_chunk);
        }
    }

    return;
}

void __far *krealloc(void __far *addr, unsigned int new_size) {
    unsigned int heap_chunk_ptr = SEGMENTOF(addr);
    struct heap_chunk_t __far *heap_chunk;
    char __far *new_ptr;

    if (!addr)
        return kalloc(new_size);

    if (!new_size) {
        kfree(addr);
        return (void __far*)0;
    }

    heap_chunk_ptr -= HEAP_CHUNK_SIZE;
    heap_chunk = FARPTR(heap_chunk_ptr,0);

    if ((new_ptr = kalloc(new_size)) == 0)
        return (void __far*)0;

    /* convert size to paragraphs */
    new_size = PARA(new_size);

    if (heap_chunk->size > new_size)
        kmemcpy(new_ptr, addr, new_size << 4);
    else
        kmemcpy(new_ptr, addr, heap_chunk->size << 4);

    kfree(addr);

    return new_ptr;
}
