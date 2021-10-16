#include<stdint.h>
#include<ptrdef.h>
#include<bios/io.h>
#include<lib/klib.h>
#include<api/stack.h>
#include<api/exep.h>

extern void int23_dispatch(void);
static char buffer[256];
static int echo_to_printer = 0;

static void handle_control_c(unsigned char key) {
    if (key =='C'-'@') {
        kputs("^C\r\n");
        int23_dispatch();
    } else if (key == 'P'-'@') {
        echo_to_printer = !echo_to_printer;
    }
}

static void check_control_c(void) {
    unsigned char key = bios_status() & 0xFF;
    /* check if ctrl+c, ctrl+p or ctrl+s in keyboard buffer */
    switch (key) {
        case 'C'-'@':
        case 'P'-'@':
            /* discard key from buffer */
            bios_getchar();
            /* do appropriate action */
            handle_control_c(key);
            break;
        case 'S'-'@':
            /* discard key from buffer */
            bios_getchar();
            /* wait for user to press key */
            handle_control_c(bios_getchar());
            /* DOS quirks: extended key codes don't get discarted, subsequent ctrl+c isn't detected */
    }
}

static void putchar(char c) {
    bios_putchar(c);
    if (echo_to_printer) bios_lpt_putchar(0,c);
}

/* int 21h ah=0x01 */
void getchar_echo(void) {
    check_control_c();
    putchar(last_sp->al = bios_getchar());
}

/* int 21h ah=0x02 */
void putchar(void) {
    check_control_c();
    putchar(last_sp->dl);
}

/* int 21h ah=0x03 */
void aux_getchar(void) {
    check_control_c();
    last_sp->al = bios_com_getchar(0);
}

/* int 21h ah=0x04 */
void aux_putchar(void) {
    check_control_c();
    bios_com_putchar(0, last_sp->dl);
}

/* int 21h ah=0x05 */
void prn_putchar(void) {
    check_control_c();
    bios_lpt_putchar(0, last_sp->dl);
}

/* int 21h ah=0x06 */
void direct_io(void) {
    if (last_sp->dl == 0xFF) {
        unsigned int status = bios_status();
        if (status) {
            last_sp->flags &= ~ZF;
            last_sp->al = bios_getchar();
        } else {
            last_sp->flags |= ZF;
            last_sp->al = 0;
        }
    } else {
        putchar(last_sp->al);
    }
}

/* int 21h ah=0x07 */
void direct_input(void) {
    last_sp->al = bios_getchar();
}

/* int 21h ah=0x08 */
void getchar_no_echo(void) {
    check_control_c();
    last_sp->al = bios_getchar();
}

/* int 21h ah=0x09 */
void puts(void) {
    char far *str = FARPTR(last_sp->ds, last_sp->dx);
    unsigned int i;
    for (i=0; str[i] != '$'; i++) {
        check_control_c();
        putchar(str[i]);
    }
}

/* int 21h ah=0x0A */
void gets(void) {
    unsigned int i;
    /* get pointer to user input buffer containing previous input */
    unsigned char far *oldbuf = FARPTR(last_sp->ds, last_sp->dx);
    /* get length of old input and new input */
    uint8_t newlen = oldbuf[0], oldlen = oldbuf[1];
    /* abort if buffer size is zero */
    if (!newlen) return;
    /* set starting position */
    uint8_t newpos = 0, oldpos = 0;
    /* point original input buffer to start of page */
    oldbuf += 2;
    while (1) {
        /* get next key including extended key code */
        check_control_c();
        unsigned int key = bios_getchar();
        if (!key) {
            check_control_c();
            key |= bios_getchar() << 8;
        }
        switch (key) {
            case 'F'-'@': /* ctrl+f */
                break;
            case '\b': /* backspace */
            case '\t': /* horizontal tab */
            case '\n': /* line feed */
            case '\r': /* carriage return */
                buffer[newpos++] = 13;
                for (i = 0; i < newpos; i++)
                    oldbuf[i] = buffer[i];
                oldbuf -= 2;
                oldbuf[1] = newpos;
                return;
            case '\e': /* escape */
            case 127: /* delete */
            default:
                if (newpos < newlen - 1) {
                } else {
                    putchar('\a'); /* beep */
                }
        }
    }
}

/* int 21h ah=0x0B */
void con_status(void) {
    check_control_c();
    last_sp->al = bios_status() ? 0xFF : 0;
}

/* int 21h ah=0x0C */
void con_flush(void) {
    bios_flush();
    switch (last_sp->al) {
    case 1:
        getchar_echo();
        break;
    case 6:
        direct_io();
        break;
    case 7:
        direct_input();
        break;
    case 8:
        getchar_no_echo();
        break;
    case 10:
        gets();
        break;
    }
    return;
}
