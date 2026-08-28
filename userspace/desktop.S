.section .text
.code64
.global _start
.type _start, @function

/* Compact 8x8 monochrome glyph bitmaps, row-major, bit 7 first. */
.set GLYPH_J, 0x003844040404043e
.set GLYPH_P, 0x004040407c44447c
.set GLYPH_E, 0x007c40407840407c
.set GLYPH_N, 0x004242464a526242
.set GLYPH_Z, 0x007e20100804027e
.set GLYPH_H, 0x004242427e424242
.set GLYPH_S, 0x007c02023c40403e
.set GLYPH_C, 0x003e40404040403e
.set GLYPH_O, 0x003c42424242423c
.set GLYPH_T, 0x001818181818187e
.set GLYPH_G, 0x003c42424e40423c
.set GLYPH_U, 0x003c424242424242
.set GLYPH_A, 0x0042427e42422418
.set GLYPH_L, 0x007e404040404040
.set GLYPH_I, 0x003c18181818183c

/*
 * SB Desktop bootstrap userspace surface.
 * Display hardware remains behind stable display syscalls. Keyboard input is
 * polled through the kernel input service until the event subsystem exists.
 */
_start:
    /* Query display geometry: width[63:32], height[31:16]. */
    mov $3, %rax
    int $0x80
    test %rax, %rax
    jz .fallback_loop

    mov %rax, %r12
    shr $32, %rax
    mov %eax, %r13d          /* width */
    mov %r12, %rax
    shr $16, %rax
    movzwl %ax, %r14d         /* height */

    /* Initial desktop background. */
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

    /* Main desktop window. */
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

    /* Bottom dock. */
    xor %edi, %edi
    mov %r14d, %esi
    sub $72, %esi
    mov %r13d, %edx
    mov $72, %r10d
    mov $0x16202C, %r8d
    mov $5, %rax
    int $0x80

    /* First-boot modal geometry. */
    mov %r13d, %eax
    sub $560, %eax
    shr $1, %eax
    mov %eax, %r10d          /* modal x */
    mov %r14d, %eax
    sub $360, %eax
    shr $1, %eax
    mov %eax, %r11d          /* modal y */
    mov $0, %r15d             /* selected language: 0=JP,1=EN,2=ZH,3=ES */

    /* Modal. */
    mov $5, %rax
    mov %r10d, %edi
    mov %r11d, %esi
    mov $560, %edx
    mov $360, %r8d
    mov $0x313B4A, %r9d
    /* syscall ABI arg3 is r10, arg4 is r8; restore explicit values below. */
    mov $360, %r10d
    mov $0x313B4A, %r8d
    int $0x80

    /* Header strip. */
    mov $5, %rax
    mov %r10d, %edi
    add $24, %edi
    mov %r11d, %esi
    add $24, %esi
    mov $512, %edx
    mov $52, %r10d
    mov $0x3A485A, %r8d
    int $0x80

    /* "LANG" label. */
    mov %r13d, %eax
    sub $256, %eax
    shr $1, %eax
    mov %r10d, %edi
    add $36, %edi
    mov %r11d, %esi
    add $38, %esi
    mov $GLYPH_L, %rdx
    mov $0xE9F2FF, %r10d
    mov $7, %rax
    int $0x80
    mov %edi, %eax
    add $10, %eax
    mov %eax, %edi
    mov $GLYPH_A, %rdx
    int $0x80
    add $10, %edi
    mov $GLYPH_N, %rdx
    int $0x80
    add $10, %edi
    mov $GLYPH_G, %rdx
    int $0x80

    /* Draw initial choices and continue button. */
    call .draw_selector
    jmp .input_loop

/* Draw four language buttons, current selection highlighted, plus labels. */
.draw_selector:
    /* JP */
    mov $5, %rax
    mov %r13d, %eax
    sub $224, %eax
    shr $1, %eax
    mov %eax, %edi
    mov %r11d, %esi
    add $92, %esi
    mov $112, %edx
    mov $48, %r10d
    mov $0x27313E, %r8d
    mov $5, %rax
    int $0x80
    /* EN */
    mov %r13d, %eax
    sub $224, %eax
    shr $1, %eax
    add $116, %eax
    mov %eax, %edi
    mov %r11d, %esi
    add $92, %esi
    mov $112, %edx
    mov $48, %r10d
    mov $0x27313E, %r8d
    mov $5, %rax
    int $0x80
    /* ZH */
    mov %r13d, %eax
    sub $224, %eax
    shr $1, %eax
    mov %eax, %edi
    mov %r11d, %esi
    add $152, %esi
    mov $112, %edx
    mov $48, %r10d
    mov $0x27313E, %r8d
    mov $5, %rax
    int $0x80
    /* ES */
    mov %r13d, %eax
    sub $224, %eax
    shr $1, %eax
    add $116, %eax
    mov %eax, %edi
    mov %r11d, %esi
    add $152, %esi
    mov $112, %edx
    mov $48, %r10d
    mov $0x27313E, %r8d
    mov $5, %rax
    int $0x80

    /* Selection accent. */
    cmp $0, %r15d
    jne .selector_not_jp
    mov %r13d, %eax
    sub $224, %eax
    shr $1, %eax
    mov %eax, %edi
    mov %r11d, %esi
    add $92, %esi
    jmp .selector_accent
.selector_not_jp:
    cmp $1, %r15d
    jne .selector_not_zh
    mov %r13d, %eax
    sub $224, %eax
    shr $1, %eax
    add $116, %eax
    mov %eax, %edi
    mov %r11d, %esi
    add $92, %esi
    jmp .selector_accent
.selector_not_zh:
    cmp $2, %r15d
    jne .selector_es
    mov %r13d, %eax
    sub $224, %eax
    shr $1, %eax
    mov %eax, %edi
    mov %r11d, %esi
    add $152, %esi
    jmp .selector_accent
.selector_es:
    mov %r13d, %eax
    sub $224, %eax
    shr $1, %eax
    add $116, %eax
    mov %eax, %edi
    mov %r11d, %esi
    add $152, %esi
.selector_accent:
    mov $112, %edx
    mov $48, %r10d
    mov $0x536F8A, %r8d
    mov $5, %rax
    int $0x80

    /* JP label. */
    mov %r13d, %eax
    sub $224, %eax
    shr $1, %eax
    add $48, %eax
    mov %eax, %edi
    mov %r11d, %esi
    add $112, %esi
    mov $GLYPH_J, %rdx
    mov $0xE9F2FF, %r10d
    mov $7, %rax
    int $0x80
    add $10, %edi
    mov $GLYPH_P, %rdx
    int $0x80

    /* EN label. */
    mov %r13d, %eax
    sub $224, %eax
    shr $1, %eax
    add $164, %eax
    mov %eax, %edi
    mov %r11d, %esi
    add $112, %esi
    mov $GLYPH_E, %rdx
    mov $0xE9F2FF, %r10d
    mov $7, %rax
    int $0x80
    add $10, %edi
    mov $GLYPH_N, %rdx
    int $0x80

    /* ZH label. */
    mov %r13d, %eax
    sub $224, %eax
    shr $1, %eax
    add $48, %eax
    mov %eax, %edi
    mov %r11d, %esi
    add $172, %esi
    mov $GLYPH_Z, %rdx
    mov $0xE9F2FF, %r10d
    mov $7, %rax
    int $0x80
    add $10, %edi
    mov $GLYPH_H, %rdx
    int $0x80

    /* ES label. */
    mov %r13d, %eax
    sub $224, %eax
    shr $1, %eax
    add $164, %eax
    mov %eax, %edi
    mov %r11d, %esi
    add $172, %esi
    mov $GLYPH_E, %rdx
    mov $0xE9F2FF, %r10d
    mov $7, %rax
    int $0x80
    add $10, %edi
    mov $GLYPH_S, %rdx
    int $0x80

    /* Continue button. */
    mov %r13d, %eax
    sub $240, %eax
    shr $1, %eax
    mov %eax, %edi
    mov %r11d, %esi
    add $232, %esi
    mov $240, %edx
    mov $44, %r10d
    mov $0x536F8A, %r8d
    mov $5, %rax
    int $0x80

    /* "CONTINUE". */
    mov %r13d, %eax
    sub $78, %eax
    shr $1, %eax
    mov %eax, %edi
    mov %r11d, %esi
    add $250, %esi
    mov $0x3e40404040403e, %rdx
    mov $0xE9F2FF, %r10d
    mov $7, %rax
    int $0x80                         /* C */
    add $10, %edi
    mov $GLYPH_O, %rdx
    int $0x80
    add $10, %edi
    mov $GLYPH_N, %rdx
    int $0x80
    add $10, %edi
    mov $GLYPH_T, %rdx
    int $0x80
    add $10, %edi
    mov $GLYPH_I, %rdx
    int $0x80
    add $10, %edi
    mov $GLYPH_N, %rdx
    int $0x80
    add $10, %edi
    mov $GLYPH_U, %rdx
    int $0x80
    add $10, %edi
    mov $GLYPH_E, %rdx
    int $0x80
    ret

.input_loop:
    mov $6, %rax
    int $0x80
    test %rax, %rax
    jz .input_idle

    /* Ignore break codes; accept PS/2 make codes. */
    test $0x80, %al
    jnz .input_idle
    cmp $0x48, %al
    je .select_up
    cmp $0x50, %al
    je .select_down
    cmp $0x1c, %al
    je .select_continue
    jmp .input_idle

.select_up:
    test %r15d, %r15d
    jz .input_idle
    dec %r15d
    call .draw_selector
    jmp .input_idle

.select_down:
    cmp $3, %r15d
    jae .input_idle
    inc %r15d
    call .draw_selector
    jmp .input_idle

.select_continue:
    /* Transition to the ordinary desktop surface. Selection is volatile until
     * the configuration/filesystem service provides atomic persistence. */
    mov $4, %rax
    mov $0x0C1018, %rdi
    int $0x80

    mov $5, %rax
    xor %edi, %edi
    xor %esi, %esi
    mov %r13d, %edx
    mov $56, %r10d
    mov $0x182230, %r8d
    int $0x80

    mov %r13d, %eax
    sub $760, %eax
    test %eax, %eax
    jns .desktop_window_x
    xor %eax, %eax
.desktop_window_x:
    mov %eax, %edi
    mov $96, %esi
    mov $720, %edx
    mov $460, %r10d
    mov $0x202A38, %r8d
    mov $5, %rax
    int $0x80

    xor %edi, %edi
    mov %r14d, %esi
    sub $72, %esi
    mov %r13d, %edx
    mov $72, %r10d
    mov $0x16202C, %r8d
    mov $5, %rax
    int $0x80

    /* Selected locale badge in the top bar. */
    cmp $0, %r15d
    je .badge_jp
    cmp $1, %r15d
    je .badge_en
    cmp $2, %r15d
    je .badge_zh

    mov $0xE9F2FF, %r10d
    mov $7, %rax
    mov %r13d, %eax
    sub $72, %eax
    mov %eax, %edi
    mov $20, %esi
    mov $GLYPH_E, %rdx
    int $0x80
    add $10, %edi
    mov $GLYPH_S, %rdx
    int $0x80
    jmp .desktop_loop

.badge_jp:
    mov $0xE9F2FF, %r10d
    mov $7, %rax
    mov $20, %edi
    mov $20, %esi
    mov $GLYPH_J, %rdx
    int $0x80
    add $10, %edi
    mov $GLYPH_P, %rdx
    int $0x80
    jmp .desktop_loop

.badge_en:
    mov $0xE9F2FF, %r10d
    mov $7, %rax
    mov $20, %edi
    mov $20, %esi
    mov $GLYPH_E, %rdx
    int $0x80
    add $10, %edi
    mov $GLYPH_N, %rdx
    int $0x80
    jmp .desktop_loop

.badge_zh:
    mov $0xE9F2FF, %r10d
    mov $7, %rax
    mov $20, %edi
    mov $20, %esi
    mov $GLYPH_Z, %rdx
    int $0x80
    add $10, %edi
    mov $GLYPH_H, %rdx
    int $0x80

.desktop_loop:
    mov $0, %rax
    int $0x80
    pause
    jmp .desktop_loop

.input_idle:
    mov $0, %rax
    int $0x80
    pause
    jmp .input_loop

.fallback_loop:
    pause
    jmp .fallback_loop

.size _start, .-_start
.section .note.GNU-stack,"",@progbits
