.section .multiboot_header
.align 8
header_start:
.long 0xE85250D6
.long 0
.long header_end - header_start
.long -(0xE85250D6 + 0 + (header_end - header_start))
/* Multiboot2 framebuffer request: 1024x768x32. The bootloader may choose
 * another suitable mode; the kernel consumes the actual framebuffer tag. */
.word 5
.word 0
.long 20
.long 1024
.long 768
.long 32
.long 0
/* End tag. */
.word 0
.word 0
.long 8
header_end:

.section .text
.code32
.global _start
.global stack_top
.global pml4
.global pdpt
.global pd
.type _start, @function
_start:
    cli
    cld
    movl %eax, multiboot_magic_saved
    movl %ebx, multiboot_info_saved
    movl $stack_top, %esp

    movl $pml4, %edi
    xorl %eax, %eax
    movl $(1024 * 3), %ecx
    rep stosl

    movl $pdpt, %eax
    orl $0x3, %eax
    movl %eax, pml4

    movl $pd, %eax
    orl $0x3, %eax
    movl %eax, pdpt

    movl $pd, %edi
    movl $0x83, %eax
    movl $512, %ecx
1:
    movl %eax, (%edi)
    addl $0x200000, %eax
    addl $8, %edi
    loop 1b

    movl %cr4, %eax
    orl $0x20, %eax
    movl %eax, %cr4

    movl $pml4, %eax
    movl %eax, %cr3

    movl $0xC0000080, %ecx
    rdmsr
    orl $0x100, %eax
    wrmsr

    movl %cr0, %eax
    orl $0x80000000, %eax
    movl %eax, %cr0

    lgdt gdt64_descriptor
    ljmp $0x18, $long_mode_entry

.code64
long_mode_entry:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    xorw %ax, %ax
    movw %ax, %fs
    movw %ax, %gs

    movl multiboot_magic_saved, %edi
    movl multiboot_info_saved, %esi
    cld
    call kmain

.hang:
    hlt
    jmp .hang

.section .rodata
.align 8
gdt64:
    .quad 0x0000000000000000
    .quad 0x00CF9A000000FFFF
    .quad 0x00CF92000000FFFF
    .quad 0x00209A0000000000
gdt64_descriptor:
    .word gdt64_descriptor - gdt64 - 1
    .long gdt64
    .long 0

.section .bss
.align 4
multiboot_magic_saved:
    .long 0
multiboot_info_saved:
    .long 0

.align 16
stack_bottom:
    .skip 16384
stack_top:

.align 4096
pml4:
    .skip 4096
pdpt:
    .skip 4096
pd:
    .skip 4096

.section .note.GNU-stack,"",@progbits
