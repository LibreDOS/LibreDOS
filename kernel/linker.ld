OUTPUT_FORMAT(binary)
ENTRY(_start)

SECTIONS
{
    io_stack = 0x0800;
    dsk_stack = 0x0B00;
    . = dsk_stack;

    .text : {
        *(.mb_header)
        *(.text*)
    }

    .data : {
        *(.data*)
        *(.rodata*)
        data_end = .;
    }

    .bss : {
        *(COMMON)
        *(.bss*)
        bss_end = .;
    }
}
