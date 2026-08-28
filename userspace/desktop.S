.section .text
.code64
.global _start
.type _start, @function

/* SB Desktop bootstrap userspace surface.
 * The initial ABI deliberately uses privileged display syscalls rather than
 * exposing hardware mappings directly. A shared-surface compositor can replace
 * this transport without changing the shell's logical drawing model.
 */
_start:
    /* Query display geometry: returns width[63:32], height[31:16], bpp[15:8]. */
    mov $3, %rax
    int $0x80
    test %rax, %rax
    jz .fallback_loop

    mov %rax, %r12
    shr $32, %rax
    mov %eax, %r13d          /* width */
    mov %r12, %rax
    shr $16, %rax
    movzwl %ax, %r14d        /* height */

    /* Clear background: RGB packed in arg0. */
    mov $4, %rax
    mov $0x0C1018, %rdi
    int $0x80

    /* Top bar. */
    mov $5, %rax
    xor %edi, %edi
    xor %esi, %esi
    mov %r13d, %edx
    mov $56, %r10d
    mov $0x182230, %r8d
    int $0x80

    /* Main window. */
    mov %r13d, %eax
    sub $760, %eax
    test %eax, %eax
    jns .window_x_ready
    xor %eax, %eax
.window_x_ready:
    mov %eax, %edi
    mov $96, %esi
    mov $720, %edx
    mov $460, %r10d
    mov $0x202A38, %r8d
    mov $5, %rax
    int $0x80

    /* First-boot modal dialog. */
    mov %r13d, %eax
    sub $560, %eax
    shr $1, %eax
    mov %eax, %edi
    mov %r14d, %eax
    sub $360, %eax
    shr $1, %eax
    mov %eax, %esi
    mov $560, %edx
    mov $360, %r10d
    mov $0x313B4A, %r8d
    mov $5, %rax
    int $0x80

    /* Dialog header. */
    mov %edi, %eax
    add $24, %eax
    mov %eax, %edi
    add $24, %esi
    mov $512, %edx
    mov $52, %r10d
    mov $0x3A485A, %r8d
    mov $5, %rax
    int $0x80

    /* Japanese option. */
    mov %esi, %eax
    add $72, %eax
    mov %eax, %esi
    mov $112, %edx
    mov $48, %r10d
    mov $0x3F566B, %r8d
    mov $5, %rax
    int $0x80

    /* English option. */
    add $60, %esi
    mov $112, %edx
    mov $48, %r10d
    mov $0x27313E, %r8d
    mov $5, %rax
    int $0x80

    /* Chinese option. */
    add $60, %esi
    mov $112, %edx
    mov $48, %r10d
    mov $0x27313E, %r8d
    mov $5, %rax
    int $0x80

    /* Spanish option. */
    add $60, %esi
    mov $112, %edx
    mov $48, %r10d
    mov $0x27313E, %r8d
    mov $5, %rax
    int $0x80

    /* Continue button. */
    add $68, %esi
    mov $240, %edx
    mov $44, %r10d
    mov $0x536F8A, %r8d
    mov $5, %rax
    int $0x80

    /* Bottom dock. */
    xor %edi, %edi
    mov %r14d, %esi
    sub $72, %esi
    mov %r13d, %edx
    mov $72, %r10d
    mov $0x16202C, %r8d
    mov $5, %rax
    int $0x80

    /* Keep the first desktop process alive while the scheduler/window system
     * layers are still being developed. */
.loop:
    mov $0, %rax
    int $0x80
    pause
    jmp .loop

.fallback_loop:
    pause
    jmp .fallback_loop

.size _start, .-_start
.section .note.GNU-stack,"",@progbits
