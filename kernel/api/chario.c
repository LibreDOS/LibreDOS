#include<stdint.h>
#include<stdbool.h>
#include<ptrdef.h>
#include<bios/io.h>
#include<api/stack.h>
#include<api/chario.h>

extern void int23_dispatch(void);
static char buffer[256];
static bool echo_to_printer = false;
static unsigned int linepos = 0;

static bool handle_control_c(char key) {
    switch (key) {
        case 'C'-'@':
            bios_putchar('^');
            bios_putchar('C');
            bios_putchar('\r');
            bios_putchar('\n');
            linepos = 0;
            int23_dispatch(); /* never returns */

        case 'P'-'@':
            echo_to_printer = !echo_to_printer;
            return true;

        case 'S'-'@':
            key = bios_getchar();
            if ((key == 'C'-'@') || (key == 'P'-'@')) handle_control_c(key);
    }

    return false;
}

static unsigned int status(void) {
    unsigned int key;
    char ascii;

    while (true) {
        key = bios_status();
        ascii = key & 0xFF;

        /* check if ctrl+c, ctrl+p or ctrl+s in keyboard buffer */
        if ((ascii == 'C'-'@') || (ascii == 'P'-'@') || (ascii == 'S'-'@')) {
            bios_getchar(); /* remove them from buffer */
            if (handle_control_c(ascii)) continue;
            key = bios_status();
	    }
        break;
    }

    return key;
}

static char getchar(void) {
    char key;

    while (true) {
        key = bios_getchar();

        /* check for ctrl+c, ctrl+p or ctrl+s */
        if (!handle_control_c(key)) break;
    }

    if (key == 'S'-'@') key = bios_getchar();

    return key;
}

void kputchar(char c) {
    if (c == '\b') {
        linepos--;
    } else if (c == '\t') {
        kputchar(' ');
        while (linepos & 7)
            kputchar(' ');
        return;
    } else if (c == '\r') {
        linepos = 0;
    } else if ((c != '\n') && (c != '\a')) {
        linepos++;
    }

    bios_putchar(c);
    if (echo_to_printer) bios_lpt_putchar(0,c);
}

void kputs(const char *str) {
    int i;
    for (i = 0; str[i]; i++)
        kputchar(str[i]);
}

/* int 21h ah=0x01 */
void getchar_echo(void) {
    kputchar(last_sp->al = getchar());
}

/* int 21h ah=0x02 */
void putchar(void) {
    status();
    kputchar(last_sp->dl);
}

/* int 21h ah=0x03 */
void aux_getchar(void) {
    status();
    last_sp->al = bios_com_getchar(0);
}

/* int 21h ah=0x04 */
void aux_putchar(void) {
    status();
    bios_com_putchar(0, last_sp->dl);
}

/* int 21h ah=0x05 */
void prn_putchar(void) {
    status();
    bios_lpt_putchar(0, last_sp->dl);
}

/* int 21h ah=0x06 */
void direct_io(void) {
    if (last_sp->dl == 0xFF) {
        unsigned int status = bios_status();
        if (status)
            last_sp->flags &= ~ZF;
        else
            last_sp->flags |= ZF;
        last_sp->al = status & 0xFF;
    } else {
        bios_putchar(last_sp->al);
    }
}

/* int 21h ah=0x07 */
void direct_input(void) {
    last_sp->al = bios_getchar();
}

/* int 21h ah=0x08 */
void getchar_no_echo(void) {
    last_sp->al = getchar();
}

/* int 21h ah=0x09 */
void puts(void) {
    char far *str = FARPTR(last_sp->ds, last_sp->dx);
    unsigned int i;

    for (i=0; str[i] != '$'; i++) {
        status();
        kputchar(str[i]);
    }
}

static int add_char_to_buffer (unsigned char key, char *buffer, uint8_t newlen, uint8_t *newpos) {
    if (*newpos < newlen - 1) {
        /* echo keys back */
        if ((key < ' ') && (key != '\t')) {
            kputchar('^');
            kputchar(key+'@');
        } else {
            kputchar(key);
        }

        /* store the key */
        buffer[(*newpos)++] = key;
        return 1;

    } else {
        /* beep if buffer full */
        kputchar('\a');
        return 0;
    }
}

static uint8_t skip_to_char (const char far *oldbuf, uint8_t oldlen, uint8_t oldpos) {
    char key;
    int count;

    /* wait for character and discard extended key codes */
    key = getchar();
    if (!key) {
        getchar();
        return oldpos;
    }

    /* find character */
    for (count = oldpos; count < oldlen; count++)
        if (oldbuf[count] == key)
            return count;

    /* do nothing, if character can't be found */
    return oldpos;
}

/* int 21h ah=0x0A */
void gets(void) {
    unsigned int i, count;
    /* get pointer to user input buffer containing previous input */
    char far *oldbuf = FARPTR(last_sp->ds, last_sp->dx);
    /* get length of old input and new input */
    uint8_t newlen = oldbuf[0], oldlen = oldbuf[1];
    /* abort if buffer size is zero */
    if (!newlen) return;
    /* set starting position */
    unsigned int initpos = linepos;
    uint8_t newpos = 0, oldpos = 0;
    bool insert = false;
    /* point original input buffer to start of page */
    oldbuf += 2;

    while (true) {
        /* get next key including extended key code */
        unsigned int key = getchar();
        if (!key) {
            key |= getchar() << 8;
        }

        switch (key) {
            case 'F'-'@': /* ctrl+f */
                break;

            case '\b': /* backspace */
            case 0x4B<<8: /* left arrow */
                /* do nothing, if no characters in buffer */
                if (!newpos) {
                    if (oldpos)
                        oldpos--;
                    break;
                }

                /* delete character */
                newpos--;
                if (oldpos)
                    oldpos--;

                /* determine amount to backtrack */
                if (linepos <= initpos) break;
                count = 1;
                if (buffer[newpos] == '\t') {
                    count = 0;
                    for (i = 1; i <= newpos; i++) {
                        if (buffer[newpos - i] == '\t') break;
                        count += buffer[newpos - i] < ' ' ? 2 : 1;
                    }
                    if (i > newpos) count += initpos;
                    count = 8 - (count & 7);
                } else if (buffer[newpos] < ' ') {
                    count = 2;
                }

                /* print stuff out */
                for (i = 0; i < count; i++)
                    kputchar('\b');
                for (i = 0; i < count; i++)
                    kputchar(' ');
                for (i = 0; i < count; i++)
                    kputchar('\b');
                break;

            case '\n': /* line feed */
                kputchar('\r');
                kputchar('\n');
                initpos = linepos;
                break;

            case '\r': /* carriage return */
                buffer[newpos++] = '\r';
                for (i = 0; i < newpos; i++)
                    oldbuf[i] = buffer[i];
                oldbuf -= 2;
                oldbuf[1] = newpos - 1;
                kputchar('\r');
                return;

            case '\e': /* escape */
                kputchar('\\');
                while (linepos != initpos)
                    kputchar('\b');
                kputchar('\n');
                insert = false;
                newpos = oldpos = 0;
                break;

            case 0x52<<8: /* insert */
                insert = !insert;
                break;

            case 0x53<<8: /* delete */
                if (oldpos < oldlen)
                    oldpos++;
                break;

            case 0x4D<<8: /* right arrow */
            case 0x3B<<8: /* F1 */
                if (oldpos >= oldlen)
                    break;
                add_char_to_buffer(oldbuf[oldpos++],buffer,newlen,&newpos);
                break;

            case 0x3C<<8: /* F2 */
                count = skip_to_char(oldbuf,oldlen,oldpos);
                /* insert keys into buffer */
                for (i = oldpos; i < count; i++)
                    if (!add_char_to_buffer(oldbuf[i],buffer,newlen,&newpos))
                        break;
                oldpos = count;
                break;

            case 0x3D<<8: /* F3 */
                for (i = oldpos; i < oldlen; i++)
                    if (!add_char_to_buffer(oldbuf[i],buffer,newlen,&newpos))
                        break;
                oldpos = oldlen;
                break;

            case 0x3E<<8: /* F4 */
                oldpos = skip_to_char(oldbuf,oldlen,oldpos);
                break;

            case 0x3F<<8: /* F5 */
                /* set current input as previous input */
                for (i = 0; i < newpos; i++)
                    oldbuf[i] = buffer[i];
                oldlen = newpos;

                /* reset input */
                kputchar('@');
                while (linepos != initpos)
                    kputchar('\b');
                kputchar('\n');
                insert = false;
                newpos = oldpos = 0;
                break;

            default:
                /* turn F6 into ctrl+z */
                if (key == (0x40 << 8))
                    key = 'Z'-'@';
                /* turn F7 into null */
                if (key == (0x41 << 8))
                    key = '\0';
                /* discard unhandled extended key codes */
                if (key >> 8)
                    break;
               /* store character and increment previous input position, if not insert mode and more characters there */
               if (add_char_to_buffer(key,buffer,newlen,&newpos) && (oldpos < oldlen) && !insert)
                   oldpos++;
        }
    }
}

/* int 21h ah=0x0B */
void con_status(void) {
    last_sp->al = status() ? 0xFF : 0;
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
