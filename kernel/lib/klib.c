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
    bios_puts(buf + i);

    return;
}

static char *hex_to_ascii_tab = "0123456789abcdef";

void kprn_x(unsigned long x) {
    int i;
    char buf[17];

    for (i = 0; i < 17; i++)
        buf[i] = 0;

    if (!x) {
        bios_puts("0x0");
        return;
    }

    for (i = 15; x; i--) {
        buf[i] = hex_to_ascii_tab[(x % 16)];
        x = x / 16;
    }

    i++;
    bios_puts("0x");
    bios_puts(buf + i);

    return;
}

void *kmemcpy(void *dest, void *src, unsigned int count) {
    unsigned int i;
    char *destptr = (char *)dest;
    char *srcptr = (char *)src;

    for (i = 0; i < count; i++)
        destptr[i] = srcptr[i];

    return dest;
}

extern symbol bss_end;
static unsigned int memory_base;
#define KRNL_MEMORY_BASE ((unsigned int)(memory_base + 0x10) & ((unsigned int)(0xfff0)))
#define KALLOC_MAX_SIZE ((unsigned int)((((unsigned int)(0xffff)) - KRNL_MEMORY_BASE) + 1))

struct heap_chunk_t {
    int free;
    unsigned int size;
    unsigned int prev_chunk;
    int padding[5];
};

void init_kalloc(void) {
    /* creates the first memory chunk */
    struct heap_chunk_t *root_chunk;

    memory_base = (unsigned int)bss_end;

    root_chunk = (struct heap_chunk_t *)KRNL_MEMORY_BASE;

    root_chunk->free = 1;
    root_chunk->size = KALLOC_MAX_SIZE - sizeof(struct heap_chunk_t);
    root_chunk->prev_chunk = 0;

    return;
}

void *kalloc(unsigned int size) {
    /* search for a big enough, free heap chunk */
    struct heap_chunk_t *heap_chunk = (struct heap_chunk_t *)KRNL_MEMORY_BASE;
    struct heap_chunk_t *new_chunk;
    struct heap_chunk_t *next_chunk;
    unsigned int heap_chunk_ptr;
    char *area;
    unsigned int i;

    /* keep allocations 16 byte aligned */
    size = (size + 0x10) & 0xfff0;

    for (;;) {
        if ((heap_chunk->free) && (heap_chunk->size > (size + sizeof(struct heap_chunk_t)))) {
            /* split off a new heap_chunk */
            new_chunk = (struct heap_chunk_t *)((unsigned int)heap_chunk + size + sizeof(struct heap_chunk_t));
            new_chunk->free = 1;
            new_chunk->size = heap_chunk->size - (size + sizeof(struct heap_chunk_t));
            new_chunk->prev_chunk = (unsigned int)heap_chunk;
            /* resize the old chunk */
            heap_chunk->free = !heap_chunk->free;
            heap_chunk->size = size;
            /* tell the next chunk where the old chunk is now */
            next_chunk = (struct heap_chunk_t *)((unsigned int)new_chunk + new_chunk->size + sizeof(struct heap_chunk_t));
            next_chunk->prev_chunk = (unsigned int)new_chunk;
            area = (char *)((unsigned int)heap_chunk + sizeof(struct heap_chunk_t));
            break;
        } else {
            heap_chunk_ptr = (unsigned int)heap_chunk;
            heap_chunk_ptr += heap_chunk->size + sizeof(struct heap_chunk_t);
            if (heap_chunk_ptr >= KALLOC_MAX_SIZE)
                return (void *)0;
            heap_chunk = (struct heap_chunk_t *)heap_chunk_ptr;
            continue;
        }
    }

    /* zero out memory */
    for (i = 0; i < size; i++)
        area[i] = 0;

    return (void *)area;
}

void kfree(void *addr) {
    unsigned int heap_chunk_ptr = (unsigned int)addr;
    struct heap_chunk_t *heap_chunk, *next_chunk, *prev_chunk;

    heap_chunk_ptr -= sizeof(struct heap_chunk_t);
    heap_chunk = (struct heap_chunk_t *)heap_chunk_ptr;

    heap_chunk_ptr += heap_chunk->size + sizeof(struct heap_chunk_t);
    next_chunk = (struct heap_chunk_t *)heap_chunk_ptr;

    prev_chunk = (struct heap_chunk_t *)heap_chunk->prev_chunk;

    /* flag chunk as free */
    heap_chunk->free = 1;

    if ((unsigned int)next_chunk >= KALLOC_MAX_SIZE)
        goto skip_next_chunk;

    /* if the next chunk is free as well, fuse the chunks into a single one */
    if (next_chunk->free) {
        heap_chunk->size += next_chunk->size + sizeof(struct heap_chunk_t);
        /* update next chunk ptr */
        next_chunk = (struct heap_chunk_t *)((unsigned int)next_chunk + next_chunk->size + sizeof(struct heap_chunk_t));
        /* update new next chunk's prev to ourselves */
        next_chunk->prev_chunk = (unsigned int)heap_chunk;
    }

skip_next_chunk:
    /* if the previous chunk is free as well, fuse the chunks into a single one */
    if (prev_chunk) {       /* if its not the first chunk */
        if (prev_chunk->free) {
            prev_chunk->size += heap_chunk->size + sizeof(struct heap_chunk_t);
            /* notify the next chunk of the change */
            if ((unsigned int)next_chunk < KALLOC_MAX_SIZE)
                next_chunk->prev_chunk = (unsigned int)prev_chunk;
        }
    }

    return;
}

void *krealloc(void *addr, unsigned int new_size) {
    unsigned int heap_chunk_ptr = (unsigned int)addr;
    struct heap_chunk_t *heap_chunk;
    char *new_ptr;

    if (!addr)
        return kalloc(new_size);

    if (!new_size) {
        kfree(addr);
        return (void *)0;
    }

    heap_chunk_ptr -= sizeof(struct heap_chunk_t);
    heap_chunk = (struct heap_chunk_t *)heap_chunk_ptr;

    if ((new_ptr = kalloc(new_size)) == 0)
        return (void *)0;

    /* keep allocations 16 byte aligned */
    new_size = (new_size + 0x10) & 0xfff0;

    if (heap_chunk->size > new_size)
        kmemcpy(new_ptr, addr, new_size);
    else
        kmemcpy(new_ptr, addr, heap_chunk->size);

    kfree(addr);

    return (void *)new_ptr;
}
