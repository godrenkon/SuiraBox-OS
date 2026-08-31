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
/* Four-byte padding brings the following end tag to the required 8-byte boundary. */
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

    /* Preserve Multiboot2 handoff values before touching scratch registers. */
    movl %eax, multiboot_magic_saved
    movl %ebx, multiboot_info_saved

    /* QEMU debugcon breadcrumb: unlike UART, this requires no UART setup. */
    movw $0x402, %dx
    movb $'A', %al
    outb %al, %dx

    /* Initialize COM1 so the kernel's normal serial console is usable. */
    movw $0x3F9, %dx
    xorb %al, %al
    outb %al, %dx
    movw $0x3FB, %dx
    movb $0x80, %al
    outb %al, %dx
    movw $0x3F8, %dx
    movb $0x03, %al
    outb %al, %dx
    movw $0x3F9, %dx
    xorb %al, %al
    outb %al, %dx
    movw $0x3FB, %dx
    movb $0x03, %al
    outb %al, %dx
    movw $0x3FA, %dx
    movb $0xC7, %al
    outb %al, %dx
    movw $0x3FC, %dx
    movb $0x0B, %al
    outb %al, %dx
    movw $0x3F8, %dx
    movb $'A', %al
    outb %al, %dx

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

    /* Enable long mode and NX page permissions before any NX PTE is used. */
    movl $0xC0000080, %ecx
    rdmsr
    orl $0x900, %eax
    wrmsr

    movl %cr0, %eax
    orl $0x80000000, %eax
    movl %eax, %cr0

    /* Re-use both diagnostic channels after paging is established. */
    movw $0x402, %dx
    movb $'B', %al
    outb %al, %dx
    movw $0x3F8, %dx
    movb $'B', %al
    outb %al, %dx

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

    movw $0x402, %dx
    movb $'C', %al
    outb %al, %dx
    movw $0x3F8, %dx
    movb $'C', %al
    outb %al, %dx

    movl multiboot_magic_saved, %edi
    movl multiboot_info_saved, %esi
    cld
    movw $0x402, %dx
    movb $'D', %al
    outb %al, %dx
    movw $0x3F8, %dx
    movb $'D', %al
    outb %al, %dx
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
