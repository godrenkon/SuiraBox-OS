ENTRY(_start)

SECTIONS
{
    __kernel_start = .;
    . = 1M;

    .text ALIGN(4K) :
    {
        KEEP(*(.multiboot_header))
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

    . = ALIGN(4K);
    __kernel_end = .;
}
