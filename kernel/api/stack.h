#ifndef __STACK_H__
#define __STACK_H__

#define CF 0x0001
#define PF 0x0004
#define AF 0x0010
#define ZF 0x0040
#define SF 0x0080
#define TF 0x0100
#define IF 0x0200
#define DF 0x0400
#define OF 0x0800

__attribute__((packed)) struct stack_frame_t {
    union {
        uint16_t ax;
        __attribute__((packed)) struct {
            uint8_t al;
            uint8_t ah;
        };
    };
    union {
        uint16_t bx;
        __attribute__((packed)) struct {
            uint8_t bl;
            uint8_t bh;
        };
    };
    union {
        uint16_t cx;
        __attribute__((packed)) struct {
            uint8_t cl;
            uint8_t ch;
        };
    };
    union {
        uint16_t dx;
        __attribute__((packed)) struct {
            uint8_t dl;
            uint8_t dh;
        };
    };
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
