# SuiraBox Boot Bring-up

## Phase 1 goal

Get a minimal x86_64 kernel booting in QEMU and verify it through GitHub Actions.

## Current boot path

```text
QEMU
  -> firmware/boot environment
  -> GRUB Multiboot2
  -> SB bootstrap assembly
  -> x86_64 long mode
  -> kmain()
  -> serial console
```

The first bootstrap currently uses GRUB Multiboot2 to reduce bootloader complexity while the kernel is being brought up. A dedicated UEFI-oriented SB bootloader remains a later design target.

## First milestone

A successful boot must print:

```text
================================
        SUIRABOX OS
================================
SB Kernel v0.1
Architecture: x86_64
Boot protocol: Multiboot2 OK
Kernel initialized.
Phase 1 bootstrap complete.
```

## Why serial output first?

Serial output makes boot behavior observable in headless QEMU and CI. Graphical output will be added after the basic kernel startup path is reliable.

## Next kernel work

1. Validate boot in CI.
2. Add GDT/IDT and exception handling.
3. Add a programmable timer.
4. Discover and manage the memory map.
5. Introduce the first physical page allocator.
