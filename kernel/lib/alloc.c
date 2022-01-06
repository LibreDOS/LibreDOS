#include<stddef.h>
#include<stdint.h>
#include<stdbool.h>
#include<ptrdef.h>
#include<lib/klib.h>
#include<lib/alloc.h>

extern symbol bss_end;
static unsigned int memory_base;
static unsigned int memory_end;
static bool inited = false;
static char memerror[] = "\r\nMemory allocation error\r\n";

/* DOS-style MCB */
__attribute__((packed)) struct mcb_t {
    char type;
    unsigned int owner;
    segment_t size;
    char reserved[11];
};

#define HEAP_CHUNK_SIZE (sizeof(struct mcb_t) >> 4)

void init_knalloc(void) {
    /* get first paragraph after kernel segment */
    memory_base = PARA((uintptr_t)bss_end);

    /* get first paragraph after conventional memory */
    asm ("int $0x12" : "=a" (memory_end));
    memory_end <<= 6;

    return;
}

/* allocates a memory chunk inside the kernel segment during initialization */
void *knalloc(size_t size) {
    size_t i;
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
    struct mcb_t far *root_chunk;

    root_chunk = FARPTR(memory_base,0);

    root_chunk->type = 'Z';
    root_chunk->owner = 0;
    root_chunk->size = memory_end - (memory_base + HEAP_CHUNK_SIZE);

    inited = true;
    return;
}

void far *kfalloc(farsize_t size, unsigned int owner) {
    /* search for a big enough, free heap chunk */
    struct mcb_t far *heap_chunk = FARPTR(memory_base,0);
    struct mcb_t far *new_chunk;
    uintfar_t heap_chunk_ptr;
    char far *area;
    segment_t i;
    size_t j;

    /* convert size into paragraphs */
    segment_t paras = PARA(size);

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
                return (void far *)0;
            heap_chunk_ptr = SEGMENTOF(heap_chunk);
            heap_chunk_ptr += heap_chunk->size + HEAP_CHUNK_SIZE;
            if (heap_chunk_ptr >= memory_end)
                kpanic(memerror);
            heap_chunk = FARPTR(heap_chunk_ptr,0);
            continue;
        }
    }

    /* zero out memory */
    char far *areaseg = area;
    for (i = 0; i < (size >> 16); i++) {
        j=0;
        do {
            areaseg[i]=0;
            j++;
        } while (j);
        areaseg += (uintfar_t)FARPTR(256,0);
    }
    for (j = 0; j < (size & 0xFFFF); j++)
        areaseg[j] = 0;

    return area;
}

void kffree(void far *addr) {
    segment_t heap_chunk_ptr = SEGMENTOF(addr);
    far struct mcb_t *heap_chunk, *next_chunk, *prev_chunk;

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

void far *kfrealloc(void far *addr, farsize_t new_size) {
    segment_t heap_chunk_ptr = SEGMENTOF(addr);
    struct mcb_t far *heap_chunk;
    char far *new_ptr;

    if (!addr)
        return kfalloc(new_size,8); /* TODO: replace with current PSP? */

    if (!new_size) {
        kffree(addr);
        return (void far *)0;
    }

    heap_chunk_ptr -= HEAP_CHUNK_SIZE;
    heap_chunk = FARPTR(heap_chunk_ptr,0);

    if ((new_ptr = kfalloc(new_size,heap_chunk->owner)) == 0)
        return (void far *)0;

    /* convert size to paragraphs */
    segment_t paras = PARA(new_size);

    if (heap_chunk->size > paras)
        kfmemcpy(new_ptr, addr, paras << 4);
    else
        kfmemcpy(new_ptr, addr, heap_chunk->size << 4);

    kffree(addr);

    return new_ptr;
}
