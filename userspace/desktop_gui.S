.section .text
.code64
.global _start
.type _start, @function

/* 8x8 glyphs used by the bootstrap UI. */
.set GLYPH_J, 0x003844040404043e
.set GLYPH_P, 0x004040407c44447c
.set GLYPH_E, 0x007c40407840407c
.set GLYPH_N, 0x004242464a526242
.set GLYPH_Z, 0x007e20100804027e
.set GLYPH_H, 0x004242427e424242
.set GLYPH_S, 0x007c02023c40403e

/* State: width=r13d, height=r14d, selection=r15d, dropdown=r11d,
 * modal x=rbp, modal y=rbx. Cursor state lives in .bss. */
_start:
    /* Display info. */
    mov $3, %rax
    int $0x80
    test %rax, %rax
    jz .halt
    mov %rax, %r12
    shr $32, %rax
    mov %eax, %r13d
    mov %r12, %rax
    shr $16, %rax
    movzwl %ax, %r14d

    /* Start with English in the real select box. */
    mov $1, %r15d
    xor %r11d, %r11d

    /* Cursor starts at the centre. */
    mov %r13d, %eax
    shr $1, %eax
    mov %eax, cursor_x(%rip)
    mov %r14d, %eax
    shr $1, %eax
    mov %eax, cursor_y(%rip)
    xor %eax, %eax
    mov %eax, mouse_buttons(%rip)

    call .draw_first_boot

.input_loop:
    /* Keyboard. */
    mov $6, %rax
    int $0x80
    test %rax, %rax
    jz .poll_mouse
    test $0x80, %al
    jnz .poll_mouse
    cmp $0x39, %al
    je .toggle
    cmp $0x48, %al
    je .up
    cmp $0x50, %al
    je .down
    cmp $0x1c, %al
    je .enter

.poll_mouse:
    mov $8, %rax
    int $0x80
    test %rax, %rax
    jz .idle

    mov %rax, %r12
    /* Decode signed dx/dy from the standard 3-byte packet. */
    shr $16, %rax
    movsx %al, %eax
    add %eax, cursor_x(%rip)
    mov %r12, %rax
    shr $24, %rax
    movsx %al, %eax
    sub %eax, cursor_y(%rip)

    /* Clamp cursor to the framebuffer. */
    mov cursor_x(%rip), %eax
    test %eax, %eax
    jns .x_nonneg
    xor %eax, %eax
    mov %eax, cursor_x(%rip)
.x_nonneg:
    mov cursor_x(%rip), %eax
    mov %r13d, %edx
    dec %edx
    cmp %edx, %eax
    jle .y_clamp
    mov %edx, cursor_x(%rip)
.y_clamp:
    mov cursor_y(%rip), %eax
    test %eax, %eax
    jns .y_nonneg
    xor %eax, %eax
    mov %eax, cursor_y(%rip)
.y_nonneg:
    mov cursor_y(%rip), %eax
    mov %r14d, %edx
    dec %edx
    cmp %edx, %eax
    jle .buttons
    mov %edx, cursor_y(%rip)

.buttons:
    mov %r12, %rax
    shr $8, %rax
    and $7, %eax
    mov mouse_buttons(%rip), %edx
    mov %eax, mouse_buttons(%rip)
    /* Left-button rising edge only. */
    test $1, %eax
    jz .idle
    test $1, %edx
    jnz .idle
    call .mouse_click

.idle:
    mov $0x80, %rcx
.delay:
    dec %rcx
    jnz .delay
    jmp .input_loop

.toggle:
    xor $1, %r11d
    call .draw_select
    jmp .input_loop

.up:
    test %r11d, %r11d
    jz .input_loop
    test %r15d, %r15d
    jz .input_loop
    dec %r15d
    call .draw_select
    jmp .input_loop

.down:
    test %r11d, %r11d
    jz .input_loop
    cmp $3, %r15d
    jae .input_loop
    inc %r15d
    call .draw_select
    jmp .input_loop

.enter:
    test %r11d, %r11d
    jnz .close_select
    call .continue_desktop
    jmp .halt
.close_select:
    xor %r11d, %r11d
    call .draw_select
    jmp .input_loop

.mouse_click:
    /* Field x=(width-420)/2, y=modal+104. */
    mov %r13d, %eax
    sub $420, %eax
    shr $1, %eax
    mov cursor_x(%rip), %edx
    cmp %eax, %edx
    jl .not_field
    add $420, %eax
    cmp %eax, %edx
    jge .not_field
    mov cursor_y(%rip), %edx
    mov %ebx, %eax
    add $104, %eax
    cmp %eax, %edx
    jl .not_field
    add $52, %eax
    cmp %eax, %edx
    jge .not_field
    xor $1, %r11d
    call .draw_select
    ret

.not_field:
    test %r11d, %r11d
    jz .continue_click
    /* Open dropdown: four rows at modal+160, 44px stride. */
    mov %r13d, %eax
    sub $420, %eax
    shr $1, %eax
    mov cursor_x(%rip), %edx
    cmp %eax, %edx
    jl .close_other
    add $420, %eax
    cmp %eax, %edx
    jge .close_other
    mov cursor_y(%rip), %edx
    mov %ebx, %eax
    add $160, %eax
    mov $0, %ecx
.row_test:
    cmp %eax, %edx
    jl .next_row
    mov %eax, %r10d
    add $40, %r10d
    cmp %r10d, %edx
    jl .select_row
.next_row:
    add $44, %eax
    inc %ecx
    cmp $4, %ecx
    jl .row_test
.close_other:
    xor %r11d, %r11d
    call .draw_select
    ret

.select_row:
    mov %ecx, %r15d
    xor %r11d, %r11d
    call .draw_select
    ret

.continue_click:
    /* Continue button x=(width-240)/2, y=modal+300. */
    mov %r13d, %eax
    sub $240, %eax
    shr $1, %eax
    mov cursor_x(%rip), %edx
    cmp %eax, %edx
    jl .not_continue
    add $240, %eax
    cmp %eax, %edx
    jge .not_continue
    mov cursor_y(%rip), %edx
    mov %ebx, %eax
    add $300, %eax
    cmp %eax, %edx
    jl .not_continue
    add $44, %eax
    cmp %eax, %edx
    jge .not_continue
    call .continue_desktop
    movq $1, %rax
    movq $0, %rax
    jmp .halt
.not_continue:
    ret

.draw_first_boot:
    mov $0x0C1018, %edi
    mov $4, %rax
    int $0x80
    call .draw_frame
    call .draw_select
    ret

.draw_frame:
    /* background */
    xor %edi, %edi
    xor %esi, %esi
    mov %r13d, %edx
    mov %r14d, %r10d
    mov $0x0C1018, %r8d
    mov $5, %rax
    int $0x80

    /* top bar */
    xor %edi, %edi
    xor %esi, %esi
    mov %r13d, %edx
    mov $72, %r10d
    mov $0x16202C, %r8d
    mov $5, %rax
    int $0x80

    /* modal */
    mov %r13d, %eax
    sub $560, %eax
    shr $1, %eax
    mov %eax, %ebp
    mov %r14d, %eax
    sub $360, %eax
    shr $1, %eax
    mov %eax, %ebx
    mov %ebp, %edi
    mov %ebx, %esi
    mov $560, %edx
    mov $360, %r10d
    mov $0x313B4A, %r8d
    mov $5, %rax
    int $0x80

    /* modal inner panel */
    mov %ebp, %edi
    add $24, %edi
    mov %ebx, %esi
    add $24, %esi
    mov $512, %edx
    mov $52, %r10d
    mov $0x3A485A, %r8d
    mov $5, %rax
    int $0x80

    /* label LANG */
    mov %ebp, %edi
    add $36, %edi
    mov %ebx, %esi
    add $38, %esi
    mov $GLYPH_J, %rdx
    mov $0xE9F2FF, %r10d
    mov $7, %rax
    int $0x80
    add $10, %edi
    mov $GLYPH_P, %rdx
    int $0x80
    ret

.draw_select:
    /* select field */
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

    /* arrow */
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

    /* selected short language code */
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
    je .sel_jp
    cmp $1, %r15d
    je .sel_en
    cmp $2, %r15d
    je .sel_zh
    mov $GLYPH_E, %rdx
    int $0x80
    add $10, %edi
    mov $GLYPH_S, %rdx
    int $0x80
    jmp .select_list
.sel_jp:
    mov $GLYPH_J, %rdx
    int $0x80
    add $10, %edi
    mov $GLYPH_P, %rdx
    int $0x80
    jmp .select_list
.sel_en:
    mov $GLYPH_E, %rdx
    int $0x80
    add $10, %edi
    mov $GLYPH_N, %rdx
    int $0x80
    jmp .select_list
.sel_zh:
    mov $GLYPH_Z, %rdx
    int $0x80
    add $10, %edi
    mov $GLYPH_H, %rdx
    int $0x80

.select_list:
    test %r11d, %r11d
    jz .continue_button
    /* dropdown surface */
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

    /* four rows */
    mov %r13d, %eax
    sub $420, %eax
    shr $1, %eax
    mov %eax, %edi
    mov %ebx, %esi
    add $160, %esi
    call .row_bg
    mov %r13d, %eax
    sub $420, %eax
    shr $1, %eax
    mov %eax, %edi
    mov %ebx, %esi
    add $204, %esi
    call .row_bg
    mov %r13d, %eax
    sub $420, %eax
    shr $1, %eax
    mov %eax, %edi
    mov %ebx, %esi
    add $248, %esi
    call .row_bg
    mov %r13d, %eax
    sub $420, %eax
    shr $1, %eax
    mov %eax, %edi
    mov %ebx, %esi
    add $292, %esi
    call .row_bg

    /* selected highlight */
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

.continue_button:
    /* Continue button */
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
    ret

.row_bg:
    mov $416, %edx
    mov $40, %r10d
    mov $0x27313E, %r8d
    mov $5, %rax
    int $0x80
    ret

.continue_desktop:
    /* Clear the bootstrap modal and draw the normal desktop shell. */
    xor %edi, %edi
    xor %esi, %esi
    mov %r13d, %edx
    mov %r14d, %r10d
    mov $0x0C1018, %r8d
    mov $5, %rax
    int $0x80

    xor %edi, %edi
    xor %esi, %esi
    mov %r13d, %edx
    mov $72, %r10d
    mov $0x16202C, %r8d
    mov $5, %rax
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

    /* dock */
    xor %edi, %edi
    mov %r14d, %esi
    sub $72, %esi
    mov %r13d, %edx
    mov $72, %r10d
    mov $0x16202C, %r8d
    mov $5, %rax
    int $0x80

    /* a central application window */
    mov %r13d, %eax
    sub $520, %eax
    shr $1, %eax
    mov %eax, %edi
    mov $150, %esi
    mov $520, %edx
    mov $320, %r10d
    mov $0x313B4A, %r8d
    mov $5, %rax
    int $0x80
    ret

.halt:
    cli
.halt_loop:
    hlt
    jmp .halt_loop

.fallback:
    jmp .halt

.section .bss
.align 8
cursor_x: .long 0
cursor_y: .long 0
mouse_buttons: .long 0

.section .note.GNU-stack,"",@progbits
