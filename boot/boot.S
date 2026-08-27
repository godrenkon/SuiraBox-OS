.section .multiboot_header
.align 8
header_start:
.long 0xE85250D6
.long 0
.long header_end - header_start
.long -(0xE85250D6 + 0 + (header_end - header_start))
/* Required end tag. */
.word 0
.word 0
.long 8
header_end:

.section .text
.code32
.global _start
.type _start, @function
_start:
    cli
    movl %eax, %edi              /* Multiboot2 magic, saved for kmain. */
    movl %ebx, %esi              /* Multiboot2 information pointer. */

    movl $stack_top, %esp

    /* Clear the page tables. */
    movl $pml4, %edi
    xorl %eax, %eax
    movl $(4096 * 3), %ecx
    rep stosl

    /* PML4[0] -> PDPT. */
    movl $pdpt, %eax
    orl $0x3, %eax
    movl %eax, pml4

    /* PDPT[0] -> Page Directory. */
    movl $pd, %eax
    orl $0x3, %eax
    movl %eax, pdpt

    /* Identity-map the first 1 GiB with 2 MiB pages. */
    movl $pd, %edi
    movl $0x83, %eax              /* present | writable | 2 MiB page */
    movl $512, %ecx
1:
    movl %eax, (%edi)
    addl $0x200000, %eax
    addl $8, %edi
    loop 1b

    /* Enable PAE. */
    movl %cr4, %eax
    orl $0x20, %eax
    movl %eax, %cr4

    /* Load page table root. */
    movl $pml4, %eax
    movl %eax, %cr3

    /* Enable EFER.LME. */
    movl $0xC0000080, %ecx
    rdmsr
    orl $0x100, %eax
    wrmsr

    /* Enable paging. */
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

    movq %rsi, %rsi              /* Keep multiboot pointer. */
    movq %rdi, %rdi              /* Keep magic. */

    call kmain

.hang:
    hlt
    jmp .hang

.section .rodata
.align 8
gdt64:
    .quad 0x0000000000000000
    .quad 0x00CF9A000000FFFF   /* 32-bit code */
    .quad 0x00CF92000000FFFF   /* data */
    .quad 0x00209A0000000000   /* 64-bit code */
gdt64_descriptor:
    .word gdt64_descriptor - gdt64 - 1
    .quad gdt64

.section .bss
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
