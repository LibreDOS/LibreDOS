OUTPUT_FORMAT(binary)
ENTRY(_start)

SECTIONS
{
    . = 0x0010;

    .text : {
        *(.entry)
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
