ENTRY(_start)

SECTIONS
{
    /* PML4 slot 1: separate from the kernel's low identity-map slot 0. */
    . = 0x0000008000000000;

    .text ALIGN(4K) :
    {
        *(.text*)
    }

    .rodata ALIGN(4K) :
    {
        *(.rodata*)
    }

    .data ALIGN(4K) :
    {
        *(.data*)
    }

    .bss ALIGN(4K) :
    {
        *(COMMON)
        *(.bss*)
    }
}
