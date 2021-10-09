OUTPUT_FORMAT(binary)
ENTRY(_start)

SECTIONS
{
    . = 0x520;

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
