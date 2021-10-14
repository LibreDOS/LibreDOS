#ifndef __STACK_H__
#define __STACK_H__

__attribute__((packed)) struct stack_frame_t {
    uint16_t ax;
    uint16_t bx;
    uint16_t cx;
    uint16_t dx;
    uint16_t si;
    uint16_t di;
    uint16_t bp;
    uint16_t ds;
    uint16_t es;
    uint16_t ip;
    uint16_t cs;
    uint16_t flags;
};

extern struct stack_frame_t far *last_sp;

#endif
