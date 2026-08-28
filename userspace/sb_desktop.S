.section .text
.code64
.global _start
.type _start, @function

/* Minimal glyph set for the first-boot select box. */
.set G_J, 0x003844040404043e
.set G_P, 0x004040407c44447c
.set G_E, 0x007c40407840407c
.set G_N, 0x004242464a526242
.set G_Z, 0x007e20100804027e
.set G_H, 0x004242427e424242
.set G_S, 0x007c02023c40403e

/* r13d=width, r14d=height, r15d=selection, r11d=dropdown. */
_start:
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
    mov $1, %r15d
    xor %r11d, %r11d
    mov $1, first_boot(%rip)

    mov %r13d, %eax
    shr $1, %eax
    mov %eax, cursor_x(%rip)
    mov %r14d, %eax
    shr $1, %eax
    mov %eax, cursor_y(%rip)
    xor %eax, %eax
    mov %eax, mouse_buttons(%rip)

    call .draw_desktop_background
    call .draw_first_boot

.loop:
    /* Keyboard path. */
    mov $6, %rax
    int $0x80
    test %rax, %rax
    jz .mouse
    test $0x80, %al
    jnz .mouse

    cmp $0x39, %al
    je .key_toggle
    cmp $0x48, %al
    je .key_up
    cmp $0x50, %al
    je .key_down
    cmp $0x1c, %al
    je .key_enter

.mouse:
    mov $8, %rax
    int $0x80
    test %rax, %rax
    jz .idle
    mov %rax, %r12

    /* cursor += signed dx, cursor -= signed dy */
    shr $16, %rax
    movsx %al, %eax
    add %eax, cursor_x(%rip)
    mov %r12, %rax
    shr $24, %rax
    movsx %al, %eax
    sub %eax, cursor_y(%rip)

    /* Clamp X/Y. */
    mov cursor_x(%rip), %eax
    test %eax, %eax
    jns .cx_ok
    xor %eax, %eax
    mov %eax, cursor_x(%rip)
.cx_ok:
    mov cursor_x(%rip), %eax
    mov %r13d, %edx
    dec %edx
    cmp %edx, %eax
    jle .cy
    mov %edx, cursor_x(%rip)
.cy:
    mov cursor_y(%rip), %eax
    test %eax, %eax
    jns .cy_ok
    xor %eax, %eax
    mov %eax, cursor_y(%rip)
.cy_ok:
    mov cursor_y(%rip), %eax
    mov %r14d, %edx
    dec %edx
    cmp %edx, %eax
    jle .click
    mov %edx, cursor_y(%rip)

.click:
    mov %r12, %rax
    shr $8, %rax
    and $7, %eax
    mov mouse_buttons(%rip), %edx
    mov %eax, mouse_buttons(%rip)
    test $1, %eax
    jz .idle
    test $1, %edx
    jnz .idle
    cmp $0, first_boot(%rip)
    je .idle
    call .mouse_firstboot_click

.idle:
    mov $0x80, %rcx
.delay:
    dec %rcx
    jnz .delay
    jmp .loop

.key_toggle:
    cmp $0, first_boot(%rip)
    je .loop
    xor $1, %r11d
    call .draw_first_boot
    jmp .loop

.key_up:
    cmp $0, first_boot(%rip)
    je .loop
    test %r11d, %r11d
    jz .loop
    test %r15d, %r15d
    jz .loop
    dec %r15d
    call .draw_first_boot
    jmp .loop

.key_down:
    cmp $0, first_boot(%rip)
    je .loop
    test %r11d, %r11d
    jz .loop
    cmp $3, %r15d
    jae .loop
    inc %r15d
    call .draw_first_boot
    jmp .loop

.key_enter:
    cmp $0, first_boot(%rip)
    je .loop
    test %r11d, %r11d
    jnz .close_dropdown
    call .finish_firstboot
    jmp .loop
.close_dropdown:
    xor %r11d, %r11d
    call .draw_first_boot
    jmp .loop

.mouse_firstboot_click:
    /* Select field. */
    mov %r13d, %eax
    sub $420, %eax
    shr $1, %eax
    mov cursor_x(%rip), %edx
    cmp %eax, %edx
    jl .check_options
    add $420, %eax
    cmp %eax, %edx
    jge .check_options
    mov cursor_y(%rip), %edx
    mov modal_y(%rip), %eax
    add $104, %eax
    cmp %eax, %edx
    jl .check_options
    add $52, %eax
    cmp %eax, %edx
    jge .check_options
    xor $1, %r11d
    call .draw_first_boot
    ret

.check_options:
    test %r11d, %r11d
    jz .check_continue
    mov %r13d, %eax
    sub $420, %eax
    shr $1, %eax
    mov cursor_x(%rip), %edx
    cmp %eax, %edx
    jl .close_outside
    add $420, %eax
    cmp %eax, %edx
    jge .close_outside
    mov cursor_y(%rip), %edx
    mov modal_y(%rip), %eax
    add $160, %eax
    xor %ecx, %ecx
.option_loop:
    cmp %eax, %edx
    jl .next_option
    mov %eax, %r10d
    add $40, %r10d
    cmp %r10d, %edx
    jl .pick_option
.next_option:
    add $44, %eax
    inc %ecx
    cmp $4, %ecx
    jl .option_loop
.close_outside:
    xor %r11d, %r11d
    call .draw_first_boot
    ret
.pick_option:
    mov %ecx, %r15d
    xor %r11d, %r11d
    call .draw_first_boot
    ret

.check_continue:
    mov %r13d, %eax
    sub $240, %eax
    shr $1, %eax
    mov cursor_x(%rip), %edx
    cmp %eax, %edx
    jl .no_click
    add $240, %eax
    cmp %eax, %edx
    jge .no_click
    mov cursor_y(%rip), %edx
    mov modal_y(%rip), %eax
    add $300, %eax
    cmp %eax, %edx
    jl .no_click
    add $44, %eax
    cmp %eax, %edx
    jge .no_click
    call .finish_firstboot
.no_click:
    ret

.finish_firstboot:
    mov $0, first_boot(%rip)
    xor %r11d, %r11d
    call .draw_desktop_background
    ret

.draw_desktop_background:
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
    jns .shell_x
    xor %eax, %eax
.shell_x:
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

    /* launcher tile */
    mov $18, %edi
    mov %r14d, %esi
    sub $60, %esi
    mov $56, %edx
    mov $48, %r10d
    mov $0x536F8A, %r8d
    mov $5, %rax
    int $0x80
    ret

.draw_first_boot:
    call .draw_desktop_background
    mov %r13d, %eax
    sub $560, %eax
    shr $1, %eax
    mov %eax, %ebp
    mov %r14d, %eax
    sub $360, %eax
    shr $1, %eax
    mov %eax, %ebx
    mov %ebx, modal_y(%rip)

    mov %ebp, %edi
    mov %ebx, %esi
    mov $560, %edx
    mov $360, %r10d
    mov $0x313B4A, %r8d
    mov $5, %rax
    int $0x80

    mov %ebp, %edi
    add $24, %edi
    mov %ebx, %esi
    add $24, %esi
    mov $512, %edx
    mov $52, %r10d
    mov $0x3A485A, %r8d
    mov $5, %rax
    int $0x80

    /* Label JP acts as the language field caption. */
    mov %ebp, %edi
    add $36, %edi
    mov %ebx, %esi
    add $38, %esi
    mov $G_J, %rdx
    mov $0xE9F2FF, %r10d
    mov $7, %rax
    int $0x80
    add $10, %edi
    mov $G_P, %rdx
    int $0x80

    /* Select field. */
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

    /* Arrow region. */
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

    call .draw_value
    test %r11d, %r11d
    jz .continue

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

    mov %r13d, %eax
    sub $420, %eax
    shr $1, %eax
    mov %eax, %edi
    mov %ebx, %esi
    add $160, %esi
    call .row
    mov %r13d, %eax
    sub $420, %eax
    shr $1, %eax
    mov %eax, %edi
    mov %ebx, %esi
    add $204, %esi
    call .row
    mov %r13d, %eax
    sub $420, %eax
    shr $1, %eax
    mov %eax, %edi
    mov %ebx, %esi
    add $248, %esi
    call .row
    mov %r13d, %eax
    sub $420, %eax
    shr $1, %eax
    mov %eax, %edi
    mov %ebx, %esi
    add $292, %esi
    call .row

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

.continue:
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

.draw_value:
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
    je .vjp
    cmp $1, %r15d
    je .ven
    cmp $2, %r15d
    je .vzh
    mov $G_E, %rdx
    int $0x80
    add $10, %edi
    mov $G_S, %rdx
    int $0x80
    ret
.vjp:
    mov $G_J, %rdx
    int $0x80
    add $10, %edi
    mov $G_P, %rdx
    int $0x80
    ret
.ven:
    mov $G_E, %rdx
    int $0x80
    add $10, %edi
    mov $G_N, %rdx
    int $0x80
    ret
.vzh:
    mov $G_Z, %rdx
    int $0x80
    add $10, %edi
    mov $G_H, %rdx
    int $0x80
    ret

.row:
    mov $416, %edx
    mov $40, %r10d
    mov $0x27313E, %r8d
    mov $5, %rax
    int $0x80
    ret

.halt:
    cli
1:
    hlt
    jmp 1b

.section .bss
.align 8
cursor_x: .long 0
cursor_y: .long 0
mouse_buttons: .long 0
first_boot: .long 1
modal_y: .long 0

.section .note.GNU-stack,"",@progbits
