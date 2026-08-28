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
    xor %edi, %edi           /* x */
    xor %esi, %esi           /* y */
    mov %r13d, %edx          /* width */
    mov $56, %ecx            /* height */
    mov $0x182230, %r8d      /* RGB */
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
    mov $460, %ecx
    mov $0x202A38, %r8d
    mov $5, %rax
    int $0x80

    /* Left content panel. */
    mov %edi, %eax
    add $28, %eax
    mov %eax, %edi
    mov $132, %esi
    mov $300, %edx
    mov $360, %ecx
    mov $0x283443, %r8d
    mov $5, %rax
    int $0x80

    /* Right content panel. */
    mov %edi, %eax
    add $332, %eax
    mov %eax, %edi
    mov $132, %esi
    mov $360, %edx
    mov $360, %ecx
    mov $0x242F3D, %r8d
    mov $5, %rax
    int $0x80

    /* Bottom dock. */
    xor %edi, %edi
    mov %r14d, %esi
    sub $72, %esi
    mov %r13d, %edx
    mov $72, %ecx
    mov $0x16202C, %r8d
    mov $5, %rax
    int $0x80

    /* Keep the first desktop process alive while the scheduler/window system
     * layers are still being developed. */
.loop:
    mov $0, %rax             /* SB_SYS_GET_TICKS */
    int $0x80
    pause
    jmp .loop

.fallback_loop:
    pause
    jmp .fallback_loop

.size _start, .-_start
.section .note.GNU-stack,"",@progbits
