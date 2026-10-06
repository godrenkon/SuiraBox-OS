#ifdef SB_DISPLAY_PROOF
.section .text
.type sb_display_probe, @function
sb_display_probe:
    push %rbx
    push %r12
    push %r13
    push %r14
    push %r15
    lea display_info(%rip), %rdi
    mov $SB_DISPLAY_INFO_SIZE, %rsi
    xor %rdx, %rdx
    mov $SB_SYS_DISPLAY_INFO, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    cmpl $SB_DISPLAY_ABI_VERSION, display_info(%rip)
    jne lifecycle_failed
    cmpl $640, display_info+4(%rip)
    jb lifecycle_failed
    cmpl $480, display_info+8(%rip)
    jb lifecycle_failed
    cmpl $SB_DISPLAY_PIXEL_RGB888, display_info+12(%rip)
    jne lifecycle_failed
    cmpl $SB_DISPLAY_PRESENT_MAX_PIXELS, display_info+16(%rip)
    jne lifecycle_failed
    cmpq $0, display_info+20(%rip)
    jne lifecycle_failed
    cmpl $0, display_info+28(%rip)
    jne lifecycle_failed
    # Read-only and unmapped output pointers must be rejected.
    lea _start(%rip), %rdi
    mov $SB_SYS_DISPLAY_INFO, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_FAULT, %rax
    jne lifecycle_failed
    mov $1, %rdi
    mov $SB_SYS_DISPLAY_INFO, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_FAULT, %rax
    jne lifecycle_failed

    # Full-screen background, header, sidebar, content area and colored tiles.
    # This is a graphics transport proof, not a desktop or first-boot dialog.
    xor %edi, %edi
    xor %esi, %esi
    mov display_info+4(%rip), %edx
    mov display_info+8(%rip), %ecx
    mov $0x0C1018, %r8d
    call display_fill
    xor %edi, %edi
    xor %esi, %esi
    mov display_info+4(%rip), %edx
    mov $48, %ecx
    mov $0x152536, %r8d
    call display_fill
    mov $16, %edi
    mov $72, %esi
    mov $144, %edx
    mov display_info+8(%rip), %ecx
    sub $112, %ecx
    mov $0x18394B, %r8d
    call display_fill
    mov $184, %edi
    mov $72, %esi
    mov display_info+4(%rip), %edx
    sub $208, %edx
    mov display_info+8(%rip), %ecx
    sub $112, %ecx
    mov $0xE6EEF2, %r8d
    call display_fill
    mov $184, %edi
    mov $72, %esi
    mov display_info+4(%rip), %edx
    sub $208, %edx
    mov $36, %ecx
    mov $0x167CA4, %r8d
    call display_fill
    mov $208, %edi
    mov $136, %esi
    mov $128, %edx
    mov $128, %ecx
    mov $0x259B72, %r8d
    call display_fill
    mov $360, %edi
    mov $136, %esi
    mov $128, %edx
    mov $128, %ecx
    mov $0xF2AE43, %r8d
    call display_fill
    mov $512, %edi
    mov $136, %esi
    mov $104, %edx
    mov $128, %ecx
    mov $0x527CBD, %r8d
    call display_fill
    xor %edi, %edi
    mov display_info+8(%rip), %esi
    sub $28, %esi
    mov display_info+4(%rip), %edx
    mov $28, %ecx
    mov $0x152536, %r8d
    call display_fill
    mov display_info+4(%rip), %edi
    sub $120, %edi
    mov display_info+8(%rip), %esi
    sub $22, %esi
    mov $104, %edx
    mov $16, %ecx
    mov $0x259B72, %r8d
    call display_fill

    # Aim all failed requests at a visible header pixel; screenshot comparison
    # proves they did not partially alter it.
    movl $0, display_request+SB_DISPLAY_PRESENT_X_OFFSET(%rip)
    movl $0, display_request+SB_DISPLAY_PRESENT_Y_OFFSET(%rip)
    movl $1, display_request+SB_DISPLAY_PRESENT_WIDTH_OFFSET(%rip)
    movl $1, display_request+SB_DISPLAY_PRESENT_HEIGHT_OFFSET(%rip)
    mov $1, %rdi
    mov $SB_DISPLAY_PRESENT_SIZE, %rsi
    xor %rdx, %rdx
    mov $SB_SYS_DISPLAY_PRESENT, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_FAULT, %rax
    jne lifecycle_failed
    movw $1, display_request+SB_DISPLAY_PRESENT_FLAGS_OFFSET(%rip)
    call display_present
    cmp $SB_SYS_ERROR_INVALID, %rax
    jne lifecycle_failed
    movw $0, display_request+SB_DISPLAY_PRESENT_FLAGS_OFFSET(%rip)
    movw $2, display_request+SB_DISPLAY_PRESENT_VERSION_OFFSET(%rip)
    call display_present
    cmp $SB_SYS_ERROR_INVALID, %rax
    jne lifecycle_failed
    movw $SB_DISPLAY_ABI_VERSION, display_request+SB_DISPLAY_PRESENT_VERSION_OFFSET(%rip)
    movl $31, display_request(%rip)
    call display_present
    cmp $SB_SYS_ERROR_INVALID, %rax
    jne lifecycle_failed
    movl $SB_DISPLAY_PRESENT_SIZE, display_request(%rip)
    movl $257, display_request+SB_DISPLAY_PRESENT_WIDTH_OFFSET(%rip)
    call display_present
    cmp $SB_SYS_ERROR_LIMIT, %rax
    jne lifecycle_failed
    movl $1, display_request+SB_DISPLAY_PRESENT_WIDTH_OFFSET(%rip)
    movl $-1, display_request+SB_DISPLAY_PRESENT_X_OFFSET(%rip)
    call display_present
    cmp $SB_SYS_ERROR_INVALID, %rax
    jne lifecycle_failed
    movl $0, display_request+SB_DISPLAY_PRESENT_X_OFFSET(%rip)
    movq $1, display_request+SB_DISPLAY_PRESENT_PIXELS_OFFSET(%rip)
    call display_present
    cmp $SB_SYS_ERROR_FAULT, %rax
    jne lifecycle_failed
    lea display_message(%rip), %rdi
    mov $display_message_len, %rsi
    mov $SB_SYS_LOG_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $display_message_len, %rax
    jne lifecycle_failed
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbx
    ret

# Fill an arbitrary rectangle by staging and presenting rows in <=256 pixels.
# Input edi=x, esi=y, edx=width, ecx=height, r8d=0x00RRGGBB.
display_fill:
    push %r12
    push %r13
    push %r14
    push %r15
    mov %edi, %r12d
    mov %esi, %r13d
    mov %edx, %r14d
    mov %ecx, %r15d
    lea display_pixels(%rip), %rdi
    mov %r8d, %eax
    mov $SB_DISPLAY_PRESENT_MAX_PIXELS, %ecx
    cld
    rep stosl
    lea display_pixels(%rip), %rax
    mov %rax, display_request+SB_DISPLAY_PRESENT_PIXELS_OFFSET(%rip)
display_fill_row:
    mov %r14d, %r9d
    mov %r12d, %r11d
display_fill_chunk:
    mov %r9d, %r10d
    cmp $SB_DISPLAY_PRESENT_MAX_PIXELS, %r10d
    jbe display_fill_size_ready
    mov $SB_DISPLAY_PRESENT_MAX_PIXELS, %r10d
display_fill_size_ready:
    mov %r11d, display_request+SB_DISPLAY_PRESENT_X_OFFSET(%rip)
    mov %r13d, display_request+SB_DISPLAY_PRESENT_Y_OFFSET(%rip)
    mov %r10d, display_request+SB_DISPLAY_PRESENT_WIDTH_OFFSET(%rip)
    call display_present
    test %rax, %rax
    jne lifecycle_failed
    add %r10d, %r11d
    sub %r10d, %r9d
    jnz display_fill_chunk
    inc %r13d
    dec %r15d
    jnz display_fill_row
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    ret
display_present:
    lea display_request(%rip), %rdi
    mov $SB_DISPLAY_PRESENT_SIZE, %rsi
    xor %rdx, %rdx
    mov $SB_SYS_DISPLAY_PRESENT, %rax
    int $SB_SYSCALL_VECTOR
    ret

.section .data
.balign 8
display_request:
    .long SB_DISPLAY_PRESENT_SIZE
    .word SB_DISPLAY_ABI_VERSION, 0
    .long 0, 0, 1, 1
    .quad 0
.section .rodata
display_message:
    .ascii "Userspace: display surface present and rejection lifecycle OK\r\n"
.set display_message_len, . - display_message
.section .bss
.balign 8
display_info:
    .skip SB_DISPLAY_INFO_SIZE
display_pixels:
    .skip SB_DISPLAY_PRESENT_MAX_PIXELS * 4
.section .text
#endif
