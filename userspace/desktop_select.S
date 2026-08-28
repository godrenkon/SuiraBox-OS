.section .text
.code64
.global _start
.type _start, @function

.set GLYPH_J, 0x003844040404043e
.set GLYPH_P, 0x004040407c44447c
.set GLYPH_E, 0x007c40407840407c
.set GLYPH_N, 0x004242464a526242
.set GLYPH_Z, 0x007e20100804027e
.set GLYPH_H, 0x004242427e424242
.set GLYPH_S, 0x007c02023c40403e
.set GLYPH_O, 0x003c42424242423c
.set GLYPH_C, 0x003e40404040403e
.set GLYPH_T, 0x001818181818187e
.set GLYPH_U, 0x003c424242424242
.set GLYPH_I, 0x003c18181818183c
.set GLYPH_L, 0x007e404040404040
.set GLYPH_A, 0x0042427e42422418

/* SB Desktop first-boot selector.
 * One real select-box control is rendered. Space opens/closes the option list,
 * Up/Down changes the highlighted option, Enter accepts/closes and a second
 * Enter from the closed control continues to the desktop.
 * r13d=width, r14d=height, r15d=selection, r11d=dropdown open flag.
 * rbp=modal x, rbx=modal y.
 */
_start:
    mov $3, %rax
    int $0x80
    test %rax, %rax
    jz .fallback_loop

    mov %rax, %r12
    shr $32, %rax
    mov %eax, %r13d
    mov %r12, %rax
    shr $16, %rax
    movzwl %ax, %r14d
    xor %r15d, %r15d
    xor %r11d, %r11d

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
    jns .main_x
    xor %eax, %eax
.main_x:
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

    mov %r13d, %eax
    sub $560, %eax
    shr $1, %eax
    mov %eax, %ebp
    mov %r14d, %eax
    sub $360, %eax
    shr $1, %eax
    mov %eax, %ebx

    mov $5, %rax
    mov %ebp, %edi
    mov %ebx, %esi
    mov $560, %edx
    mov $360, %r10d
    mov $0x313B4A, %r8d
    int $0x80

    mov $5, %rax
    mov %ebp, %edi
    add $24, %edi
    mov %ebx, %esi
    add $24, %esi
    mov $512, %edx
    mov $52, %r10d
    mov $0x3A485A, %r8d
    int $0x80

    /* LANG */
    mov %ebp, %edi
    add $36, %edi
    mov %ebx, %esi
    add $38, %esi
    mov $GLYPH_L, %rdx
    mov $0xE9F2FF, %r10d
    mov $7, %rax
    int $0x80
    add $10, %edi
    mov $GLYPH_A, %rdx
    int $0x80
    add $10, %edi
    mov $GLYPH_N, %rdx
    int $0x80
    add $10, %edi
    mov $GLYPH_G, %rdx
    int $0x80

    call .draw_select

.input_loop:
    mov $6, %rax
    int $0x80
    test %rax, %rax
    jz .input_idle
    test $0x80, %al
    jnz .input_idle

    cmp $0x39, %al          /* Space: open/close select */
    je .toggle_dropdown
    cmp $0x48, %al          /* Up */
    je .select_up
    cmp $0x50, %al          /* Down */
    je .select_down
    cmp $0x1c, %al          /* Enter */
    je .select_enter
    jmp .input_idle

.toggle_dropdown:
    xor $1, %r11d
    call .draw_select
    jmp .input_idle

.select_up:
    test %r11d, %r11d
    jz .input_idle
    test %r15d, %r15d
    jz .input_idle
    dec %r15d
    call .draw_select
    jmp .input_idle

.select_down:
    test %r11d, %r11d
    jz .input_idle
    cmp $3, %r15d
    jae .input_idle
    inc %r15d
    call .draw_select
    jmp .input_idle

.select_enter:
    test %r11d, %r11d
    jz .continue_desktop
    xor %r11d, %r11d
    call .draw_select
    jmp .input_idle

/* Render the select box and its option list. */
.draw_select:
    /* Field frame. */
    mov %r13d, %eax
    sub $420, %eax
    shr $1, %eax
    mov %eax, %edi
    mov %ebx, %esi
    add $104, %esi
    mov $420, %edx
    mov $52, %r10d
    mov $0x202A38, %r8d
    mov $5, %rax
    int $0x80

    /* Arrow segment. */
    mov %r13d, %eax
    sub $64, %eax
    shr $1, %eax
    add $166, %eax
    mov %eax, %edi
    mov %ebx, %esi
    add $112, %esi
    mov $52, %edx
    mov $36, %r10d
    mov $0x3A485A, %r8d
    mov $5, %rax
    int $0x80

    /* Selected value. */
    mov %r13d, %eax
    sub $420, %eax
    shr $1, %eax
    add $20, %eax
    mov %eax, %edi
    mov %ebx, %esi
    add $124, %esi
    mov $0xE9F2FF, %r10d
    mov $7, %rax
    cmp $0, %r15d
    je .value_jp
    cmp $1, %r15d
    je .value_en
    cmp $2, %r15d
    je .value_zh

    mov $GLYPH_E, %rdx
    int $0x80
    add $10, %edi
    mov $GLYPH_S, %rdx
    int $0x80
    jmp .after_value
.value_jp:
    mov $GLYPH_J, %rdx
    int $0x80
    add $10, %edi
    mov $GLYPH_P, %rdx
    int $0x80
    jmp .after_value
.value_en:
    mov $GLYPH_E, %rdx
    int $0x80
    add $10, %edi
    mov $GLYPH_N, %rdx
    int $0x80
    jmp .after_value
.value_zh:
    mov $GLYPH_Z, %rdx
    int $0x80
    add $10, %edi
    mov $GLYPH_H, %rdx
    int $0x80
.after_value:

    test %r11d, %r11d
    jz .draw_continue

    /* Option list opens directly below the field. */
    mov %r13d, %eax
    sub $420, %eax
    shr $1, %eax
    mov %eax, %edi
    mov %ebx, %esi
    add $158, %esi
    mov $420, %edx
    mov $176, %r10d
    mov $0x171D27, %r8d
    mov $5, %rax
    int $0x80

    /* Four option rows. */
    mov %r13d, %eax
    sub $420, %eax
    shr $1, %eax
    mov %eax, %edi
    mov %ebx, %esi
    add $160, %esi
    call .draw_option_background
    mov %r13d, %eax
    sub $420, %eax
    shr $1, %eax
    mov %eax, %edi
    mov %ebx, %esi
    add $204, %esi
    call .draw_option_background
    mov %r13d, %eax
    sub $420, %eax
    shr $1, %eax
    mov %eax, %edi
    mov %ebx, %esi
    add $248, %esi
    call .draw_option_background
    mov %r13d, %eax
    sub $420, %eax
    shr $1, %eax
    mov %eax, %edi
    mov %ebx, %esi
    add $292, %esi
    call .draw_option_background

    /* Highlight selected row. */
    mov %r15d, %eax
    imul $44, %eax, %eax
    add $160, %eax
    mov %r13d, %edx
    sub $420, %edx
    shr $1, %edx
    mov %edx, %edi
    mov %ebx, %esi
    add %eax, %esi
    mov $416, %edx
    mov $40, %r10d
    mov $0x536F8A, %r8d
    mov $5, %rax
    int $0x80

    /* Option labels. */
    mov %r13d, %eax
    sub $420, %eax
    shr $1, %eax
    add $20, %eax
    mov %eax, %edi
    mov %ebx, %esi
    add $176, %esi
    mov $0xE9F2FF, %r10d
    mov $7, %rax
    mov $GLYPH_J, %rdx
    int $0x80
    add $10, %edi
    mov $GLYPH_P, %rdx
    int $0x80

    mov %r13d, %eax
    sub $420, %eax
    shr $1, %eax
    add $20, %eax
    mov %eax, %edi
    mov %ebx, %esi
    add $220, %esi
    mov $GLYPH_E, %rdx
    int $0x80
    add $10, %edi
    mov $GLYPH_N, %rdx
    int $0x80

    mov %r13d, %eax
    sub $420, %eax
    shr $1, %eax
    add $20, %eax
    mov %eax, %edi
    mov %ebx, %esi
    add $264, %esi
    mov $GLYPH_Z, %rdx
    int $0x80
    add $10, %edi
    mov $GLYPH_H, %rdx
    int $0x80

    mov %r13d, %eax
    sub $420, %eax
    shr $1, %eax
    add $20, %eax
    mov %eax, %edi
    mov %ebx, %esi
    add $308, %esi
    mov $GLYPH_E, %rdx
    int $0x80
    add $10, %edi
    mov $GLYPH_S, %rdx
    int $0x80

.draw_continue:
    mov %r13d, %eax
    sub $240, %eax
    shr $1, %eax
    mov %eax, %edi
    mov %ebx, %esi
    add $300, %esi
    mov $240, %edx
    mov $44, %r10d
    mov $0x536F8A, %r8d
    mov $5, %rax
    int $0x80

    mov %r13d, %eax
    sub $78, %eax
    shr $1, %eax
    mov %eax, %edi
    mov %ebx, %esi
    add $318, %esi
    mov $GLYPH_C, %rdx
    mov $0xE9F2FF, %r10d
    mov $7, %rax
    int $0x80
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

.draw_option_background:
    /* x in edi, y in esi. */
    mov $416, %edx
    mov $40, %r10d
    mov $0x27313E, %r8d
    mov $5, %rax
    int $0x80
    ret

.continue_desktop:
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
    jns .desktop_x
    xor %eax, %eax
.desktop_x:
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

    cmp $0, %r15d
    je .desktop_jp
    cmp $1, %r15d
    je .desktop_en
    cmp $2, %r15d
    je .desktop_zh

    mov %r13d, %eax
    sub $72, %eax
    mov %eax, %edi
    mov $20, %esi
    mov $GLYPH_E, %rdx
    mov $0xE9F2FF, %r10d
    mov $7, %rax
    int $0x80
    add $10, %edi
    mov $GLYPH_S, %rdx
    int $0x80
    jmp .desktop_loop

.desktop_jp:
    mov $20, %edi
    mov $20, %esi
    mov $GLYPH_J, %rdx
    mov $0xE9F2FF, %r10d
    mov $7, %rax
    int $0x80
    add $10, %edi
    mov $GLYPH_P, %rdx
    int $0x80
    jmp .desktop_loop
.desktop_en:
    mov $20, %edi
    mov $20, %esi
    mov $GLYPH_E, %rdx
    mov $0xE9F2FF, %r10d
    mov $7, %rax
    int $0x80
    add $10, %edi
    mov $GLYPH_N, %rdx
    int $0x80
    jmp .desktop_loop
.desktop_zh:
    mov $20, %edi
    mov $20, %esi
    mov $GLYPH_Z, %rdx
    mov $0xE9F2FF, %r10d
    mov $7, %rax
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
