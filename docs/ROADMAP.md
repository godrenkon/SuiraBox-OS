# SuiraBox OS Roadmap

## Phase 0 — Project foundation

- [x] Public repository
- [x] Initial architecture document
- [ ] License decision
- [ ] Contribution guidelines
- [ ] Code of Conduct
- [ ] Build system
- [ ] GitHub Actions CI

## Phase 1 — Bootable kernel

- [ ] x86_64 boot environment
- [ ] Linker script
- [ ] Kernel entry point
- [ ] Serial console
- [ ] Basic framebuffer/text output
- [ ] Boot success test in QEMU

Milestone: SB boots in QEMU and prints a kernel banner.

## Phase 2 — CPU and interrupts

- [ ] GDT
- [ ] IDT
- [ ] CPU exception handlers
- [ ] Interrupt controller
- [ ] Programmable timer
- [ ] Basic interrupt test

Milestone: SB handles timer interrupts and CPU exceptions safely.

## Phase 3 — Memory

- [ ] Memory map discovery
- [ ] Physical page allocator
- [ ] x86_64 page tables
- [ ] Virtual memory manager
- [ ] Kernel heap
- [ ] User address spaces

Milestone: kernel can allocate/free pages and isolate address spaces.

## Phase 4 — Processes and scheduler

- [ ] Kernel threads
- [ ] Process abstraction
- [ ] Context switching
- [ ] Preemptive scheduling
- [ ] Sleep/wakeup
- [ ] Resource accounting

Milestone: multiple kernel/user execution contexts can run independently.

## Phase 5 — Syscalls and userspace

- [ ] Syscall entry/exit
- [ ] Handles / file descriptors
- [ ] Process creation
- [ ] Thread creation
- [ ] Basic IPC
- [ ] Minimal userspace runtime

Milestone: a user program can start, perform a syscall, and exit.

## Phase 6 — Storage

- [ ] Block device abstraction
- [ ] VFS
- [ ] Initial filesystem support
- [ ] File I/O
- [ ] Directory operations
- [ ] Cache layer

Milestone: userspace can create, read, write, and delete files.

## Phase 7 — Networking

- [ ] NIC abstraction
- [ ] QEMU virtual NIC driver
- [ ] Ethernet
- [ ] IPv4
- [ ] UDP
- [ ] TCP
- [ ] Socket API
- [ ] IPv6
- [ ] Firewall/policy framework
- [ ] Network monitor

Milestone: userspace can communicate over a network from QEMU.

## Phase 8 — Graphics and drivers

- [ ] Driver framework
- [ ] Input devices
- [ ] Display abstraction
- [ ] GPU architecture research
- [ ] Initial graphics API
- [ ] Physical hardware driver strategy

Milestone: graphical userspace can display and receive input.

## Phase 9 — Runtime

- [ ] Native userspace runtime
- [ ] JVM research/integration plan
- [ ] JVM-compatible OS primitives
- [ ] Runtime resource management

Milestone: a controlled JVM environment can run on SB.

## Phase 10 — SB Hub

- [ ] Desktop shell
- [ ] App management
- [ ] Terminal
- [ ] File manager
- [ ] System settings
- [ ] Performance monitor

## Phase 11 — Minecraft platform

- [ ] Minecraft instance manager
- [ ] Java runtime manager
- [ ] Mod management
- [ ] Mod-loader management
- [ ] Minecraft launcher integration
- [ ] Minecraft performance profiles
- [ ] Minecraft diagnostics

## Phase 12 — Minecraft Server platform

- [ ] Server manager
- [ ] Server profiles
- [ ] Easy install/remove
- [ ] Backup/restore
- [ ] Multi-server management
- [ ] Server monitoring
- [ ] Resource policies
- [ ] Network policies

## Phase 13 — Optimization and validation

- [ ] Kernel benchmark suite
- [ ] Network benchmark suite
- [ ] Storage benchmark suite
- [ ] Scheduler benchmark suite
- [ ] Minecraft workload benchmarks
- [ ] Regression tracking
- [ ] Hardware compatibility testing

Only measured improvements should become SB-specific optimizations.
