# SuiraBox OS — Master Recovery & Complete Construction Specification

> **Document status:** Canonical master specification.
>
> **Purpose:** This file is the practical backup of the entire SB Desktop project plan. If the original project context, chat history, planning notes, or an individual AI session is lost, a competent developer or AI should be able to reconstruct the intended product, architecture, development order, invariants, interfaces, tests, UX rules, release process, and future expansion from this document plus the source tree.
>
> **Current product:** SB Desktop.
>
> **Current development rule:** Do not spend active development effort designing a separate CUI-only edition. SB Desktop must still contain a real terminal because command-line operation is a required capability.
>
> **Important distinction:** This is a target specification, not a claim that every item below is already implemented. Implementation status must always be determined from the source tree and CI/runtime tests.

---

# 0. How to use this document

This document has five jobs:

1. Preserve the original product vision.
2. Define the technical architecture.
3. Define the exact order in which the system should be constructed.
4. Tell a developer/AI how to verify each stage.
5. Act as a recovery document if project context is lost.

Before making an architectural change:

1. Read this document.
2. Inspect the current repository.
3. Determine which phase the implementation is actually in.
4. Do not assume planned features exist.
5. Find the smallest unfinished prerequisite.
6. Implement and test that prerequisite.
7. Update this document only when the architecture or plan genuinely changes.

Never use this document as evidence that an unimplemented feature works.

---

# 1. Product identity and philosophy

## 1.1 Name

Project name: **SuiraBox OS**.

Short name: **SB**.

Primary current edition: **SB Desktop**.

The project is open-source and is intended to be usable, lightweight, configurable, extensible and understandable.

## 1.2 Core idea

The OS should behave more like a well-designed toolkit than a monolithic appliance:

- install a small base system;
- keep optional components outside the base installation where practical;
- let the user choose what is installed and enabled;
- expose detailed settings without making them confusing;
- provide a graphical interface for normal users;
- retain a real terminal for advanced users;
- recover from ordinary failures without unnecessarily killing the whole system;
- make serious failures diagnostically useful;
- remove unnecessary implementation/data overhead without sacrificing correctness;
- make the system modular enough that hardware and product variants can later branch from the same stable foundation.

## 1.3 User-centered principle

The intended experience is approximately:

`Power on → boot → kernel → hardware discovery → memory → core services → graphics/input → desktop → first-run language selector → usable system`

The user should not need to understand bootloaders, kernel internals, package databases or shell syntax merely to start using the computer.

Advanced users must still have access to:

- terminal;
- detailed settings;
- diagnostics;
- logs;
- developer controls;
- recovery tools.

## 1.4 Minimal base, expandable system

The base image should contain only what is required to boot and provide a usable desktop plus essential infrastructure.

Optional components should be installable later:

- applications;
- additional language packs;
- fonts;
- themes;
- development tools;
- multimedia components;
- optional utilities;
- hardware drivers when appropriate;
- other large non-essential resources.

This is not permission to split everything into tiny packages. Packaging boundaries must remain practical, secure and maintainable.

## 1.5 Performance philosophy

“Lightweight” means more than a small ISO.

Measure:

- boot latency;
- idle RAM;
- idle CPU;
- storage footprint;
- application startup latency;
- GUI frame latency;
- background wakeups;
- network overhead;
- package installation cost.

Never sacrifice correctness merely to remove a few bytes.

---

# 2. Absolute engineering rules

These rules apply to every subsystem.

1. Do not claim an implementation exists when only a design exists.
2. Do not claim hardware support without a reproducible test.
3. Do not call a mock UI a working feature.
4. Do not weaken CI tests to make a failing build appear successful.
5. Do not hide a kernel failure by removing its diagnostic output.
6. Do not silently delete user data.
7. Do not make a lower layer depend on a higher layer merely for convenience.
8. Keep one source of truth for every important configuration value.
9. Prefer bounded operations over infinite loops.
10. Avoid busy polling when event-driven or blocking mechanisms are available.
11. A normal application error must not automatically become a kernel panic.
12. A kernel panic must not be used for a normal recoverable error.
13. RSOD is reserved for exceptionally severe early-boot/display/recovery/integrity conditions.
14. BSOD is reserved for unsafe/unrecoverable kernel/system state.
15. All user-visible strings must be localizable.
16. Initial GUI languages are Japanese, English, Chinese and Spanish.
17. The first-run language selection is graphical, not terminal-based.
18. The first-run language control is a select/drop-down box.
19. The terminal remains a required part of SB Desktop.
20. Network operations must have explicit error/timeout behavior.
21. Optional software should not unnecessarily consume boot-time resources.
22. Configuration writes should be atomic/transactional where practical.
23. Security must be enforced at the kernel/service boundary, not only in the GUI.
24. Diagnostics must not leak passwords, tokens or private keys.
25. Temporary debug code must be removed or deliberately documented.
26. Every architectural change must have a reason.
27. Every major subsystem must have a test strategy.
28. Runtime behavior, not source-file existence, determines implementation status.

---

# 3. Master architecture

## 3.1 High-level layers

The intended stack is:

`Firmware/BIOS/UEFI`

`↓`

`Bootloader`

`↓`

`Early Kernel`

`↓`

`CPU + Memory + Interrupt Infrastructure`

`↓`

`Kernel Core Services`

`↓`

`Device / Storage / Network Subsystems`

`↓`

`Userspace Init + Services`

`↓`

`Display + Input Services`

`↓`

`Compositor + Window System`

`↓`

`Desktop Shell`

`↓`

`Settings / File Manager / Terminal / Store / Applications`

## 3.2 Dependency direction

Preferred dependency direction:

`boot → kernel primitives → kernel services → device abstractions → userspace services → GUI/applications`

A lower layer must not import GUI concepts merely to display an error.

For example:

- PMM must not depend on the Settings application.
- Network drivers must not depend on a GUI window.
- Package manager must not require a graphical shell to function.
- GUI must call userspace APIs instead of touching hardware directly.

## 3.3 Failure containment

The preferred failure boundary is:

`application failure < service failure < userspace subsystem failure < kernel subsystem failure < boot-critical failure`

Attempt recovery at the lowest appropriate boundary.

---

# 4. Repository organization

Expected ownership:

- `boot/` — boot entry, early CPU state, boot protocol handling.
- `kernel/` — privileged code and kernel subsystems.
- `userspace/` — userspace services, shell, GUI and applications.
- `docs/` — specifications, architecture notes and test plans.
- `site/` — project/public website assets.
- `.github/` — CI and release automation.
- `Makefile` — reproducible build orchestration.
- `linker.ld` — kernel image layout.

Names may change as the project evolves, but ownership boundaries must remain clear.

Recommended future kernel structure:

```text
kernel/
  arch/x86_64/
    boot/
    cpu/
    interrupt/
    paging/
    timer/
  mm/
    pmm/
    vmm/
    heap/
  sched/
  process/
  syscall/
  ipc/
  drivers/
    pci/
    input/
    storage/
    display/
    network/
  fs/
  net/
  security/
  log/
```

Recommended future userspace structure:

```text
userspace/
  init/
  services/
    config/
    display/
    input/
    network/
    package/
    update/
    log/
  gui/
    compositor/
    toolkit/
    shell/
    settings/
    filemanager/
    terminal/
    store/
  apps/
```

These are target boundaries, not requirements to create empty directories immediately.

---

# 5. Complete development roadmap

The project should progress through these phases.

## Phase 0 — Build foundation

Goal: reproducible kernel/ISO build.

Deliverables:

- toolchain definition;
- freestanding compiler flags;
- assembler configuration;
- linker script;
- Makefile targets;
- bootable ISO;
- QEMU invocation;
- serial log collection;
- CI pipeline.

Exit condition: clean checkout can produce the same class of boot artifact without undocumented manual steps.

## Phase 1 — Boot

Goal: reach kernel main safely.

Deliverables:

- boot protocol entry;
- CPU mode;
- stack;
- preserved boot information;
- early console;
- safe kernel layout.

Exit condition: QEMU boots repeatedly without memory corruption.

## Phase 2 — Memory

Goal: safe physical and virtual memory.

Order:

`PMM → VMM → kernel heap`

Exit condition: allocation, mapping, faults and cleanup tests pass.

## Phase 3 — CPU/runtime foundation

Order:

`GDT → TSS → IDT → exceptions → interrupt controller → timer → synchronization`

Exit condition: controlled exceptions and timer interrupts work.

## Phase 4 — Execution

Order:

`Scheduler → threads → processes → address spaces → syscalls → userspace init`

Exit condition: a userspace program can start, perform basic syscalls and exit cleanly.

## Phase 5 — Storage

Order:

`block devices → partitions → filesystem driver → VFS → configuration persistence`

Exit condition: files can be created/read/written/renamed/deleted and configuration survives reboot.

## Phase 6 — Hardware I/O

Order:

`keyboard → mouse → framebuffer/display → basic storage/network devices`

Exit condition: generic input/display work in QEMU and on the supported baseline hardware.

## Phase 7 — GUI

Order:

`text rendering → windows/surfaces → event system → compositor → window management → desktop shell`

Exit condition: a real interactive desktop is usable.

## Phase 8 — First-run and localization

Order:

`configuration service → locale service → language catalog → first-run popup → persistence → Settings integration`

Exit condition: clean install displays the language selector graphically and persists the choice.

## Phase 9 — Network

Order:

`NIC abstraction → Ethernet → IPv4/IPv6 → ARP/ND → routing → UDP/TCP → DNS/DHCP → sockets → network manager`

Exit condition: basic networking works through GUI and terminal.

## Phase 10 — Software distribution

Order:

`package format → package database → repository metadata → downloader → verification → transactions → GUI Store`

Exit condition: optional software can be installed and removed safely.

## Phase 11 — Hardening

Include:

- security;
- recovery;
- diagnostics;
- updates;
- rollback;
- performance;
- cleanup;
- compatibility testing.

## Phase 12 — Release

Only after all release gates pass.

---

# 6. Build system specification

## 6.1 Compiler model

Kernel code is freestanding.

Do not accidentally depend on the host OS's libc, startup files or runtime.

Compiler flags must explicitly define:

- architecture;
- ABI;
- freestanding mode;
- stack behavior as appropriate;
- position/relocation model as required;
- optimization level;
- warning policy.

## 6.2 Assembly

Boot assembly must be assembled for the exact target architecture.

Avoid mixing 32-bit and 64-bit relocation assumptions.

Any address whose size matters must be expressed using the correct assembler/linker representation.

## 6.3 Linker

The linker script must define and document:

- boot entry;
- text;
- read-only data;
- writable data;
- BSS;
- alignment;
- kernel end symbol;
- any early memory structures.

After every layout-affecting change, inspect the actual linked addresses.

## 6.4 Build targets

At minimum, conceptually support:

- `make` — normal build;
- `make clean` — remove generated build output;
- ISO creation;
- QEMU boot;
- test execution;
- artifact validation.

Exact target names may differ, but equivalent functionality must exist.

## 6.5 CI philosophy

CI is part of the OS, not decoration.

Required stages:

1. checkout
2. dependency/toolchain setup
3. compile
4. assembly
5. link
6. linker-layout validation
7. ISO generation
8. Multiboot validation
9. QEMU boot
10. serial-log collection
11. smoke assertions
12. memory tests
13. userspace tests
14. GUI tests when available
15. persistence tests
16. network tests
17. package tests
18. recovery tests
19. artifact integrity checks

A timeout is a failure until the cause is understood.

---

# 7. Bootloader and early boot

## 7.1 Responsibilities

Bootloader/early assembly should do only what is necessary to hand control safely to the kernel.

Establish:

- expected CPU mode;
- known stack;
- boot protocol information;
- required initial page tables;
- documented kernel entry ABI.

## 7.2 Memory safety

Never allow these regions to overlap:

- boot code;
- boot data;
- stack;
- page tables;
- kernel image;
- PMM metadata;
- Multiboot structures still needed;
- framebuffer reservation;
- firmware-reserved memory.

## 7.3 Boot diagnostics

Early output must be minimal, deterministic and independent of heap/GUI.

Useful checkpoints:

```text
Boot entry
CPU initialized
Stack ready
Boot information preserved
Paging ready
Kernel main entered
```

Do not make early diagnostics dependent on a subsystem that has not yet initialized.

---

# 8. Physical Memory Manager — PMM

## 8.1 Purpose

PMM owns physical page allocation.

## 8.2 Responsibilities

- know page size;
- track reserved/free pages;
- allocate physical pages;
- free physical pages;
- optionally allocate contiguous pages;
- report statistics;
- protect its own metadata.

## 8.3 Initialization strategy

Use a safe bootstrap path first if full boot memory information cannot yet be trusted.

Then integrate the full memory map without resetting active allocator state.

Critical invariant:

**A second initialization pass must never erase allocations already in use.**

## 8.4 Reservation order

Before exposing memory as free:

1. reserve firmware-reserved regions;
2. reserve bootloader structures still needed;
3. reserve kernel image;
4. reserve stack;
5. reserve page tables;
6. reserve PMM bitmap/metadata;
7. reserve framebuffer where applicable;
8. mark remaining genuinely usable memory free.

## 8.5 API concept

Equivalent operations:

```text
pmm_init()
pmm_reserve_range(start, length)
pmm_release_range(start, length)
pmm_alloc_page()
pmm_alloc_pages(count)
pmm_free_page(page)
pmm_get_stats()
```

Exact names are implementation details.

## 8.6 Invariants

- reserved page cannot be allocated;
- allocation returns aligned page;
- free returns page to allocator exactly once;
- invalid page is rejected;
- double free is detected where practical;
- metadata cannot overlap free pages;
- counters remain internally consistent.

## 8.7 PMM debugging checklist

If boot stops at PMM:

1. verify PMM entry log;
2. verify PMM static storage address;
3. verify linker layout;
4. verify bitmap does not overlap kernel/stack/page tables;
5. verify loop bounds;
6. verify page count calculation overflow;
7. verify no second initialization wipes state;
8. verify serial output itself is not blocking;
9. test a fixed tiny allocator independently;
10. compare QEMU memory size against assumptions.

## 8.8 Tests

- first allocation;
- repeated allocation;
- free/reallocate;
- exhaustion;
- invalid page;
- double free;
- reserved memory protection;
- low-memory QEMU;
- high-memory QEMU.

---

# 9. Virtual Memory Manager — VMM

## 9.1 Responsibilities

- create address spaces;
- create page tables;
- map physical pages;
- unmap pages;
- set permissions;
- destroy mappings;
- manage TLB invalidation as needed.

## 9.2 Security model

User mappings must not arbitrarily expose kernel memory.

Kernel mappings must remain valid during kernel execution.

## 9.3 Required permissions

Conceptually support:

- present/not-present;
- writable/read-only;
- user/kernel;
- executable/non-executable where architecture permits.

## 9.4 Fault behavior

Unmapped or prohibited access must result in a controlled page fault.

The exception subsystem decides whether the fault is:

- recoverable userspace fault;
- process termination;
- kernel panic.

---

# 10. Kernel heap

## 10.1 Purpose

Provide dynamic kernel allocations above page-level PMM.

## 10.2 Requirements

Define behavior for:

- alignment;
- zero-size allocation;
- large allocation;
- allocation failure;
- integer overflow;
- invalid free;
- repeated allocation/free;
- fragmentation.

## 10.3 Recursion rule

Early logging must not require the heap if the heap is the component being debugged.

---

# 11. CPU tables and exception handling

## 11.1 GDT

Define required kernel/user segments according to the chosen x86_64 model.

## 11.2 TSS

Provide required stack-switch and task-state facilities.

## 11.3 IDT

Install handlers for relevant CPU exceptions before enabling normal userspace execution.

## 11.4 Exception context

Capture when available:

- exception vector;
- CPU error code;
- instruction pointer;
- stack pointer;
- flags;
- general registers;
- current process/thread;
- kernel build ID;
- boot/session ID.

---

# 12. Interrupts, timer and synchronization

Provide:

- interrupt controller abstraction;
- IRQ routing;
- timer source;
- interrupt-safe locking;
- wait queues/events;
- sleep/wakeup.

Never use infinite polling for normal waiting.

Timer behavior must be deterministic enough for scheduler tests.

---

# 13. Scheduler

## 13.1 Responsibilities

- maintain runnable entities;
- context switch;
- sleep/wakeup;
- priorities if implemented;
- fairness policy;
- CPU accounting.

## 13.2 GUI requirement

GUI event processing must not be starved by background tasks.

## 13.3 Failure rules

A stuck userspace process must not be able to monopolize the kernel indefinitely.

---

# 14. Threads and processes

Required lifecycle:

```text
create → initialize → runnable → running → sleeping/waiting → runnable → exit → cleanup
```

Process resources must be released after exit.

Resources include:

- address space;
- file handles;
- IPC endpoints;
- threads;
- memory allocations;
- service registrations.

---

# 15. Syscalls and userspace ABI

## 15.1 Principle

Userspace cannot directly trust kernel memory and kernel cannot blindly trust userspace input.

## 15.2 Validation

At every syscall boundary validate:

- pointers;
- lengths;
- integer overflow;
- object handles;
- ownership;
- permissions;
- buffer ranges.

## 15.3 ABI stability

The userspace ABI should be versioned so applications do not silently break after kernel updates.

## 15.4 Initial syscall families

- process/thread;
- memory;
- files;
- time;
- IPC;
- network;
- configuration/service access;
- controlled device access.

---

# 16. Userspace init and service manager

## 16.1 Goal

Start the minimum services needed for a usable desktop.

## 16.2 Service model

Services should declare:

- name;
- dependencies;
- startup conditions;
- restart policy;
- privilege requirements;
- logging destination.

## 16.3 Lazy operation

Do not permanently start optional services merely because an application is installed.

## 16.4 Failure containment

If a network service fails, attempt network-service recovery before considering broader system failure.

---

# 17. Storage architecture

Layers:

`block device → partition → filesystem driver → VFS → file APIs → applications`

## 17.1 Block layer

Abstract sector/block reads and writes.

## 17.2 Partition layer

Detect supported partition structures without hard-coding a particular physical disk.

## 17.3 Filesystem layer

Support a practical initial filesystem and keep the VFS independent of its implementation.

## 17.4 VFS

Provide common concepts:

- path;
- file;
- directory;
- metadata;
- permissions;
- handles.

## 17.5 File operations

At minimum:

- create;
- open;
- read;
- write;
- seek;
- close;
- rename;
- delete;
- enumerate directory;
- stat/metadata;
- flush where required.

---

# 18. Persistent configuration service

This is a major cross-system dependency.

## 18.1 One source of truth

The same configuration API must serve:

- first-run setup;
- Settings;
- locale;
- keyboard layout;
- network manager;
- power settings;
- performance settings;
- desktop preferences.

## 18.2 Setting schema

Every setting should define:

- stable key;
- type;
- default;
- valid range/values;
- scope;
- owner;
- persistence policy;
- migration version where needed.

## 18.3 Atomicity

Prefer:

`write temporary → validate → fsync/commit as appropriate → atomic replace`

or an equivalent journal/transaction mechanism.

## 18.4 Corruption handling

If configuration cannot be parsed:

1. do not crash the kernel;
2. preserve a diagnostic copy when safe;
3. restore defaults or enter first-run recovery;
4. avoid infinite setup loops.

---

# 19. Input subsystem

## 19.1 Hardware abstraction

Convert device input into common events:

- key down;
- key up;
- pointer movement;
- pointer button;
- wheel;
- future touch/gesture events.

## 19.2 Focus

The window system controls input focus.

## 19.3 Keyboard layouts

Keyboard layout is configurable and must not be hard-coded into kernel logic.

---

# 20. Display subsystem

## 20.1 Baseline

Start with a generic framebuffer path.

The basic desktop should not require a vendor-specific accelerated GPU driver when a generic framebuffer is available.

## 20.2 Acceleration

Later provide a driver abstraction for accelerated rendering.

Potential future hardware families:

- Intel graphics;
- AMD graphics;
- NVIDIA consumer GPUs;
- NVIDIA professional GPUs;
- high-end workstation/server accelerators.

Do not claim support until actual driver code and tests exist.

---

# 21. GUI architecture

Layers:

`display driver → display service → compositor → window system → desktop shell → applications`

## 21.1 Toolkit primitives

Must eventually include:

- window/surface;
- label;
- text;
- text field;
- button;
- list;
- select/drop-down;
- checkbox/toggle;
- menu;
- dialog;
- notification;
- scroll container.

## 21.2 GUI event loop

The GUI must be event-driven.

Do not create a background polling loop for every widget.

## 21.3 Rendering

Support:

- clipping;
- dirty-region or equivalent redraw optimization;
- text rendering;
- font fallback;
- DPI/scaling foundation;
- localized string sizing.

---

# 22. Compositor and window system

## 22.1 Compositor responsibilities

- collect window surfaces;
- determine stacking order;
- composite visible regions;
- synchronize display updates;
- handle cursor/system surfaces where appropriate.

## 22.2 Window manager

Provide:

- create/destroy windows;
- move;
- resize;
- minimize/maximize where supported;
- focus;
- close;
- keyboard navigation.

---

# 23. Desktop shell

Minimum desktop features:

- application launcher;
- task/window management;
- status area;
- notification area;
- Settings entry;
- File Manager entry;
- Terminal entry;
- Network status;
- power/restart/shutdown controls.

Optional applications must not all start at boot.

---

# 24. FIRST BOOT — exact user experience

This is one of the most important requirements in the project.

## 24.1 What must NOT happen

The finished GUI edition must not require the user to see a debug terminal and type commands to select the initial language.

## 24.2 Correct sequence

```text
Power on
 ↓
Bootloader
 ↓
Kernel
 ↓
Core services
 ↓
Display initialization
 ↓
Compositor
 ↓
Desktop shell
 ↓
First-run check
 ↓
Language popup, if configuration is absent
 ↓
Normal desktop
```

## 24.3 Popup

The popup should be simple and lightweight:

```text
Welcome to SuiraBox

Language
[ English ▼ ]

[ Continue ]
```

The language control is explicitly a **select/drop-down box**.

## 24.4 Initial choices

- 日本語
- English
- 中文
- Español

## 24.5 Selection transaction

When Continue is pressed:

1. read selected locale;
2. verify it is supported;
3. write locale to persistent configuration;
4. activate the locale service;
5. update visible UI;
6. mark first-run complete;
7. continue normally.

## 24.6 Next boot

On the next boot:

```text
read config
 ↓
valid locale?
 ├─ yes → desktop
 └─ no → safe first-run/default recovery
```

## 24.7 Failure behavior

If the configuration file is missing, malformed or incompatible:

- never crash the kernel;
- never create an infinite reboot loop;
- safely use a default locale or show first-run again;
- retain diagnostics where useful.

---

# 25. Localization architecture

## 25.1 Stable IDs

Never scatter translated strings throughout the code.

Example IDs:

```text
ui.welcome.title
ui.language.label
ui.continue
settings.language
error.network.timeout
error.storage.read_failed
panic.kernel.page_fault
```

## 25.2 Initial languages

1. Japanese
2. English
3. Chinese
4. Spanish

## 25.3 Fallback

If a translation is missing:

`selected language → fallback language → safe identifier/default`

The UI must not render an unusable blank control because one translation is absent.

## 25.4 Text system

The text renderer must eventually handle:

- Unicode;
- different string lengths;
- fonts/glyph fallback;
- wrapping;
- alignment;
- input methods as needed.

## 25.5 Language packs

Language packs may be distributed separately from the minimal base image once the package system is mature.

---

# 26. Settings application

Settings should be detailed without becoming chaotic.

Categories:

1. System
2. Appearance
3. Display
4. Sound
5. Network
6. Keyboard & Mouse
7. Storage
8. Applications
9. Privacy & Security
10. Accounts
11. Updates
12. Language & Region
13. Performance
14. Developer
15. Recovery

Every setting should have:

- stable ID;
- current value;
- default;
- explanation;
- validation;
- reset behavior;
- persistence behavior;
- owning subsystem.

Advanced settings can be exposed without forcing ordinary users to understand them.

---

# 27. Normal errors

Normal errors should be understandable and should not look like a catastrophic kernel failure.

Example information:

```text
Something went wrong

Component: Network
Error ID: NET-0042

The connection could not be established.

[ Details ] [ Retry ] [ Close ]
```

Details should explain:

- what happened;
- likely cause when known;
- what the OS already tried;
- whether data is safe;
- what the user can do next.

---

# 28. BSOD

## 28.1 Meaning

BSOD means approximately:

> The system was operating, but the kernel determined that continuing was unsafe or impossible.

It is not an ordinary error dialog.

## 28.2 Diagnostic content

When available, include:

- stable error ID;
- failure class;
- CPU exception/vector;
- CPU error code;
- instruction pointer;
- stack pointer;
- relevant registers;
- current process/thread;
- kernel version/build ID;
- subsystem;
- boot/session ID;
- recovery status;
- diagnostic report ID.

## 28.3 User actions

Offer safe actions such as:

- restart;
- recovery mode;
- save/export diagnostic report when possible;
- view support information.

Do not pretend that a reboot fixes the underlying cause.

---

# 29. RSOD

## 29.1 Meaning

RSOD is reserved for conditions where the normal display/recovery path itself cannot be trusted, or extremely early/critical system data cannot be trusted.

Examples of intended classes:

- unrecoverable display initialization failure;
- severe early boot integrity failure;
- recovery UI unavailable at a critical point.

## 29.2 Rule

Do not use RSOD for:

- ordinary application crashes;
- normal driver errors;
- network failures;
- package failures;
- routine kernel-recoverable errors.

The visual severity must correspond to the actual severity.

---

# 30. Diagnostics and support report

## 30.1 Goal

A user who encounters a serious failure should be able to produce useful information for themselves or support.

## 30.2 Report content

Potential sections:

- SB version;
- kernel build ID;
- boot/session ID;
- hardware summary;
- CPU information;
- memory summary;
- storage summary;
- display/driver summary;
- network summary;
- active services;
- recent relevant logs;
- error IDs;
- crash/panic context;
- recovery actions.

## 30.3 Secret filtering

Never include by default:

- passwords;
- authentication tokens;
- private keys;
- session cookies;
- secrets from environment variables;
- raw credential stores.

Provide explicit user control before exporting sensitive diagnostic data.

---

# 31. Recovery architecture

Prefer the narrowest recovery boundary.

Examples:

- failed application → restart application;
- failed userspace service → restart service;
- network failure → restart network service;
- display service failure → restart display service if safe;
- failed package update → rollback package transaction;
- corrupt configuration → restore defaults/recovery copy;
- unrecoverable kernel state → panic/reboot.

Recovery itself must have bounded retries.

Do not endlessly restart a broken service.

---

# 32. Network stack

Target architecture:

`NIC driver`

`↓`

`link layer`

`↓`

`ARP / Neighbor Discovery`

`↓`

`IPv4 / IPv6`

`↓`

`routing`

`↓`

`UDP / TCP`

`↓`

`DNS / DHCP`

`↓`

`sockets`

`↓`

`network manager`

## 32.1 Initial capabilities

- loopback;
- Ethernet;
- IPv4;
- IPv6;
- ARP/ND;
- routing;
- DHCP;
- static addresses;
- DNS;
- sockets.

## 32.2 Future capabilities

- Wi-Fi drivers;
- firewall;
- VPN interfaces;
- advanced routing;
- network diagnostics;
- connection profiles.

## 32.3 GUI/CLI parity

Network configuration performed in Settings and commands performed in Terminal should ultimately use the same service/API.

---

# 33. Terminal

SB Desktop must contain a real terminal.

It is not the first-run setup mechanism, but it is a first-class advanced interface.

Eventually support:

- command execution;
- environment variables;
- pipes;
- redirection;
- filesystem commands;
- process inspection;
- network tools;
- package management;
- diagnostics;
- recovery;
- scripting.

Avoid implementing a fake terminal that merely displays canned output.

---

# 34. Package manager

Architecture:

`repository metadata → resolver → downloader → verifier → transaction engine → installed database`

## 34.1 Package identity

Each package requires:

- name/ID;
- version;
- architecture;
- dependencies;
- conflicts;
- files;
- metadata;
- integrity/signature information.

## 34.2 Transactions

Installation should conceptually be:

`resolve → download → verify → stage → validate → commit → register`

Failure before commit should leave the installed state consistent.

## 34.3 Removal

Removing an optional package must not silently remove unrelated user data.

## 34.4 Cache

Package cache cleanup must distinguish:

- active package state;
- cached package files;
- temporary downloads;
- rollback data.

---

# 35. SB Store

The GUI Store is a front end to the package system, not a second package manager.

Potential categories:

- applications;
- development tools;
- language packs;
- fonts;
- themes;
- multimedia;
- network utilities;
- desktop extensions;
- optional drivers/firmware where legally distributable.

## 35.1 Download efficiency

Use, where appropriate:

- compressed metadata;
- compressed packages;
- caching;
- resumable downloads;
- mirrors/CDN later;
- parallel downloads where beneficial.

Never weaken integrity verification for speed.

## 35.2 Minimal base principle

A user should not need to download the entire ecosystem just to install one application.

---

# 36. Driver architecture

Drivers must expose stable interfaces to the rest of the OS.

Initial priority:

1. PCI
2. framebuffer/display
3. keyboard
4. mouse
5. storage
6. Ethernet
7. USB as needed for baseline usability

Later:

- audio;
- Wi-Fi;
- accelerated graphics;
- modern storage controllers;
- additional USB/PCIe devices.

## 36.1 Graceful degradation

If an accelerated driver is unavailable:

`accelerated graphics unavailable → generic framebuffer fallback`

when technically possible.

Do not make the whole desktop fail merely because an optional acceleration feature is missing.

---

# 37. GPU and high-performance hardware strategy

The initial release should target a broad **generic x86_64 PC** baseline rather than prematurely maintaining dozens of hardware-specific images.

Future hardware compatibility may include:

- common integrated graphics;
- discrete consumer GPUs;
- professional GPUs;
- workstation hardware;
- server hardware;
- high-end accelerator systems.

NVIDIA RTX-class and professional/data-center GPU support must be treated as real driver projects, not a build flag.

When enough hardware data exists, release artifacts may be separated by hardware profile, but the common OS architecture should remain shared wherever possible.

---

# 38. Security architecture

Security enforcement belongs at privileged boundaries.

Required direction:

- kernel/userspace isolation;
- page permissions;
- syscall validation;
- process isolation;
- file permissions;
- package authenticity;
- signed repository metadata;
- secure update path;
- least privilege;
- controlled device access;
- firewall/network policy;
- sanitized diagnostics.

Never rely on a GUI toggle alone to enforce a security rule.

---

# 39. Update and rollback system

Target flow:

`check metadata → verify metadata → resolve dependencies → download → verify packages → stage → validate → activate → retain rollback path`

Requirements:

- signed metadata;
- package integrity;
- dependency resolution;
- interrupted-update recovery;
- rollback;
- clear user-visible progress;
- recovery-mode update path.

Never intentionally leave the installed OS in an untracked half-updated state.

---

# 40. Data minimization and cleanup

The desire to remove unnecessary hidden data must be implemented conservatively.

## Safe cleanup candidates

- expired temporary files;
- obsolete download cache;
- orphaned package cache;
- stale generated build artifacts where explicitly owned;
- old disposable logs according to retention policy.

## Never automatically delete

- user documents;
- unknown files;
- credentials;
- recovery metadata;
- active package state;
- data whose ownership cannot be determined.

Every cleanup rule requires an explicit ownership model.

---

# 41. Performance engineering

## 41.1 Boot

Measure each phase independently:

```text
bootloader
kernel entry
memory init
interrupt init
scheduler
userspace init
display
compositor
desktop
first-run
```

## 41.2 Idle

Measure:

- resident memory;
- CPU wakeups;
- timers;
- background services;
- storage writes;
- network activity.

## 41.3 Application launch

Track cold and warm startup.

## 41.4 GUI

Measure:

- input latency;
- frame time;
- dropped frames;
- compositor CPU usage;
- redraw area.

## 41.5 Optimization rule

Do not optimize based on intuition when instrumentation can answer the question.

---

# 42. Compatibility philosophy

Initial target:

**generic x86_64 desktop/laptop PC and QEMU**.

Compatibility should be capability-based.

Do not assume:

- a specific GPU;
- a specific disk controller;
- a specific network chipset;
- a specific amount of RAM.

Graceful fallback is preferred to hard failure.

---

# 43. Testing architecture

Every subsystem should have the strongest practical combination of:

- unit tests;
- integration tests;
- QEMU tests;
- negative tests;
- persistence tests;
- stress tests;
- performance measurements.

## 43.1 Kernel tests

Examples:

- PMM allocation;
- VMM mapping;
- heap allocation;
- exception dispatch;
- timer;
- scheduler;
- syscall validation.

## 43.2 Userspace tests

Examples:

- process start/exit;
- file operations;
- configuration writes;
- network service;
- package transactions.

## 43.3 GUI tests

Examples:

- desktop startup;
- input focus;
- button click;
- select/drop-down operation;
- dialog close;
- localization switching;
- first-run flow.

---

# 44. Mandatory end-to-end test list

Before a first major stable release, test:

1. cold boot;
2. repeated reboot;
3. shutdown;
4. clean first boot;
5. first graphical language popup;
6. language selection;
7. language persistence;
8. keyboard;
9. mouse;
10. display;
11. settings persistence;
12. file creation;
13. file reading;
14. file writing;
15. file deletion;
16. network configuration;
17. terminal;
18. package installation;
19. package removal;
20. recoverable application failure;
21. service restart;
22. diagnostic report creation;
23. recovery mode;
24. controlled kernel exception;
25. ISO integrity;
26. QEMU boot;
27. low-memory test;
28. larger-memory test;
29. update transaction;
30. rollback.

---

# 45. Release engineering

## 45.1 Release candidate

Before release:

1. freeze the candidate;
2. run complete CI;
3. inspect logs;
4. run clean-install tests;
5. test reboot persistence;
6. test first-run setup;
7. test package transactions;
8. test recovery;
9. verify checksums/signatures;
10. document known limitations.

## 45.2 Release artifacts

Potential artifacts:

- bootable ISO;
- checksums;
- signatures;
- release notes;
- installation documentation;
- recovery documentation;
- compatibility notes.

Do not publish a stable artifact with an unresolved boot-critical failure.

---

# 46. Project documentation and public distribution

The public project should eventually have:

- official website;
- source repository;
- releases;
- installation guide;
- user manual;
- developer documentation;
- architecture documentation;
- troubleshooting guide;
- security policy;
- contribution guide;
- issue templates;
- release notes;
- support/diagnostic instructions.

GitHub Pages may host the public website.

The website must distinguish clearly between:

- stable features;
- experimental features;
- planned features;
- unsupported hardware.

---

# 47. Community and official identity

Project/community name: **Suiram**.

The OS project itself is **SuiraBox OS / SB**.

Official community channels may eventually include:

- official website;
- GitHub;
- Discord;
- X/Twitter;
- documentation;
- release feed.

Do not let community infrastructure become a dependency of the OS itself.

---

# 48. AI development protocol

This project is expected to be worked on with AI assistance. Therefore AI behavior is part of the engineering specification.

## 48.1 Required loop

```text
READ DESIGN
 ↓
INSPECT CURRENT SOURCE
 ↓
IDENTIFY ACTUAL STATE
 ↓
IDENTIFY SMALLEST ROOT CAUSE / PREREQUISITE
 ↓
IMPLEMENT
 ↓
BUILD
 ↓
RUN TEST
 ↓
INSPECT OUTPUT
 ↓
FIX ROOT CAUSE
 ↓
ADD/UPDATE TEST
 ↓
DOCUMENT ARCHITECTURAL CHANGE
 ↓
REPORT EXACT STATUS
```

## 48.2 AI must not

- invent files;
- invent CI results;
- invent hardware support;
- say “implemented” when only planned;
- remove tests because they fail;
- silently replace architecture;
- hide diagnostics;
- turn an error into success by changing the expected output;
- introduce a GUI mock and call it a desktop;
- use the debug terminal as the final GUI first-run experience;
- silently delete features;
- silently delete user data.

## 48.3 When blocked

If the current toolset cannot modify a required file:

- state that limitation;
- do not claim the modification happened;
- preserve the exact proposed change in documentation/issue when useful;
- continue with tasks that are actually executable.

## 48.4 Status language

Use precise terms:

- **planned** — described but not implemented;
- **scaffolded** — basic structure exists;
- **implemented** — code exists and is believed functional;
- **build-tested** — compilation/link/image test passes;
- **runtime-tested** — behavior was exercised;
- **stable** — release criteria for that feature are satisfied.

Never collapse all five into “done.”

---

# 49. Debugging methodology

When something fails:

## Step 1 — Reproduce

Get the smallest reproducible case.

## Step 2 — Establish the boundary

Determine the last confirmed-good checkpoint.

## Step 3 — Add narrow diagnostics

Add checkpoints around the suspected operation.

## Step 4 — Inspect addresses/state

For memory-related failures inspect:

- linker map;
- symbol addresses;
- stack bounds;
- page-table addresses;
- allocator metadata;
- kernel end;
- framebuffer.

## Step 5 — Remove false leads

A loop that should take microseconds but takes 20 seconds is not automatically “a slow loop.” Check:

- blocking I/O;
- repeated faulting;
- invalid memory access;
- serial output;
- CPU mode;
- interrupt state;
- address overlap.

## Step 6 — Fix root cause

Do not permanently paper over the symptom.

## Step 7 — Regression test

The original failure must become a permanent test whenever practical.

---

# 50. Known class of early-boot hazards

This section exists specifically to prevent repeating common mistakes.

## 50.1 Relocation mismatch

If 32-bit assembly attempts to encode a 64-bit relocation, the assembler/linker may fail.

Always match:

- assembler mode;
- relocation type;
- address size;
- linker output.

## 50.2 Buffer size mismatch

If code intends to clear `N` bytes using a 32-bit store instruction, the loop count is `N / 4`, not `N`.

Every memory-clear loop must document:

- unit size;
- number of units;
- total bytes.

## 50.3 BSS/static memory collision

Never assume a static array is harmless because it is “only metadata.”

Check actual linked addresses against:

- stack;
- page tables;
- kernel image;
- boot data.

## 50.4 Double initialization

An allocator or subsystem that is already live must not be reinitialized by a later discovery pass unless its API explicitly supports state migration.

## 50.5 Debug output recursion

Do not use heap allocation to print heap-debug information before heap initialization.

## 50.6 Serial blocking

A diagnostic print routine must not make a tiny algorithm appear to hang for seconds.

## 50.7 Unbounded parsing

Every boot-provided structure parser requires:

- total-size validation;
- entry-size validation;
- maximum iteration bound;
- malformed-data handling.

---

# 51. Memory map handling

When consuming Multiboot or firmware memory maps:

1. validate total structure size;
2. validate each entry size;
3. reject entries outside the provided structure;
4. cap unreasonable counts;
5. identify usable vs reserved memory;
6. reserve the kernel and boot allocations;
7. only then expose free pages.

The parser must not assume the firmware supplied a friendly or perfectly formatted map.

---

# 52. Logging architecture

Logs should eventually be structured.

Fields:

- timestamp;
- severity;
- component;
- event ID;
- boot/session ID;
- process/thread when applicable;
- message;
- diagnostic fields.

Severity concept:

```text
TRACE
DEBUG
INFO
NOTICE
WARNING
ERROR
CRITICAL
PANIC
```

Exact names may change.

## 52.1 Log retention

Logs must not grow without bound.

Use a documented retention policy and preserve crash-critical information.

---

# 53. Power management

Eventually support:

- shutdown;
- reboot;
- sleep where hardware permits;
- wake events;
- power status;
- battery reporting on laptops.

Shutdown must flush required persistent state before powering off.

---

# 54. Accounts and permissions

Eventually provide:

- user identity;
- authentication;
- groups/roles;
- file ownership;
- permissions;
- privileged administration mechanism.

Do not make all desktop applications permanently privileged.

---

# 55. Privacy

Settings should make important privacy behavior visible.

Potential controls:

- diagnostics sharing;
- telemetry policy;
- application permissions;
- network permissions;
- device access;
- crash-report contents.

Default behavior must be documented.

---

# 56. Accessibility

GUI foundation should account for:

- keyboard-only navigation;
- focus visibility;
- scalable text;
- high-contrast capability;
- readable notifications;
- screen-reader-compatible semantics where architecture permits.

Do not make accessibility an impossible afterthought by hard-coding all UI as pixels.

---

# 57. Internationalization beyond translation

Localization is more than translating labels.

The system should eventually account for:

- date formats;
- time formats;
- decimal separators;
- sorting/collation;
- pluralization;
- timezone;
- keyboard layouts;
- text direction if later supported.

The initial release still starts with the four specified UI languages.

---

# 58. Application model

Applications should have:

- package identity;
- declared permissions/capabilities;
- executable entry point;
- metadata;
- localization resources;
- icons/assets;
- configuration namespace;
- clean uninstall behavior.

Uninstalling an application should not automatically destroy unrelated user documents.

---

# 59. File manager

A standard desktop needs a usable file manager.

Minimum capabilities:

- navigate directories;
- create directory;
- create/open files;
- rename;
- move;
- copy;
- delete with confirmation where appropriate;
- show metadata;
- search later;
- handle errors clearly.

Large operations should be asynchronous so the GUI remains responsive.

---

# 60. Notifications

Notifications are separate from catastrophic error screens.

Levels may include:

- informational;
- success;
- warning;
- error;
- critical.

A notification should provide enough context to understand what happened and where to act.

Critical system failure must not be hidden as an ordinary toast.

---

# 61. Search/indexing

If desktop search is implemented, it must be optional or carefully optimized.

Do not make an aggressive full-disk indexing daemon a mandatory idle workload without evidence that the user benefits from it.

Indexing must respect permissions and privacy.

---

# 62. Developer mode

Advanced users/developers may eventually enable:

- verbose logs;
- kernel diagnostics;
- development tools;
- tracing;
- debug symbols;
- experimental features.

Developer mode must not silently weaken security on normal installations.

---

# 63. Compatibility fallback hierarchy

When hardware acceleration or a specialized service is unavailable, attempt:

```text
preferred implementation
 ↓
compatible generic implementation
 ↓
reduced-feature fallback
 ↓
clear diagnostic error
```

Do not immediately turn a missing optional driver into a catastrophic boot failure.

---

# 64. Resource ownership rules

Every persistent resource must have an owner.

Examples:

- configuration → configuration service;
- package database → package manager;
- language catalog → localization service;
- framebuffer → display service;
- network interface state → network manager;
- user files → filesystem/user layer.

Two independent subsystems must not both believe they own the same mutable state.

---

# 65. API design rules

Public internal APIs should:

- have documented ownership;
- define failure values;
- define lifetime;
- define synchronization requirements;
- define whether calls may block;
- define privilege requirements.

Avoid APIs whose behavior changes silently based on hidden global state.

---

# 66. Concurrency rules

Every shared mutable structure needs a synchronization strategy.

Document:

- lock owner;
- lock ordering;
- interrupt context restrictions;
- whether sleeping while holding a lock is allowed;
- lifetime/reference counting.

Avoid deadlocks through explicit lock-order documentation.

---

# 67. Memory safety checklist

Before merging memory-related changes:

- alignment checked;
- integer overflow checked;
- bounds checked;
- lifetime checked;
- ownership checked;
- page permissions checked;
- free path checked;
- double-free path checked;
- low-memory path checked;
- initialization order checked;
- linker addresses checked.

---

# 68. Network safety checklist

Before merging network changes:

- input length checked;
- packet bounds checked;
- malformed packet handled;
- timeout defined;
- retry bounded;
- resource limits defined;
- privilege boundary defined;
- logs sanitized.

---

# 69. Package security checklist

Before trusting a package:

1. repository metadata is trusted/verified;
2. package identity is validated;
3. version is validated;
4. architecture is compatible;
5. dependencies are resolved;
6. integrity is verified;
7. signature/authenticity is verified where required;
8. transaction is staged safely;
9. installed state is recorded.

---

# 70. Update safety checklist

Before activation:

- package verification passed;
- dependencies resolved;
- enough storage exists;
- rollback path exists;
- configuration migration is known;
- current state is recorded.

After activation:

- boot succeeds;
- core services start;
- configuration is readable;
- rollback remains available until validation completes.

---

# 71. First-release quality bar

A release is not “done” merely because:

- it boots once;
- the GUI screenshot looks good;
- the ISO exists;
- one QEMU test passes.

A credible first major release requires:

- repeatable boot;
- stable memory management;
- stable process/user boundary;
- storage persistence;
- real GUI interaction;
- input;
- initial localization;
- terminal;
- basic networking;
- safe package installation/removal;
- understandable errors;
- diagnostic reports;
- recovery paths;
- CI coverage;
- documented hardware limitations.

---

# 72. Definition of Done — SB Desktop v1

A normal user must be able to:

1. boot supported x86_64 hardware or QEMU;
2. reach a graphical desktop without terminal setup;
3. see the first-boot language selector;
4. select Japanese, English, Chinese or Spanish;
5. press Continue;
6. enter the desktop;
7. reboot;
8. retain the selected language;
9. use keyboard and mouse;
10. configure networking;
11. open the terminal;
12. manage files;
13. install optional software;
14. remove optional software safely;
15. modify detailed settings;
16. receive normal understandable error messages;
17. inspect serious-error diagnostics;
18. use supported recovery paths;
19. update the system safely;
20. operate without unnecessary optional software consuming the base system.

Only after this foundation is stable should compatibility expansion become the primary objective.

---

# 73. Future expansion — after v1

Once SB Desktop v1 is genuinely stable, the architecture can branch toward:

- gaming optimization;
- workstation optimization;
- high-end GPU support;
- professional GPU support;
- server-oriented configurations;
- data-center hardware;
- additional architectures if justified;
- specialized lightweight installations;
- additional package ecosystem;
- richer developer tooling.

These are future branches, not current blockers for the core desktop build.

The guiding rule is:

**Finish one coherent system first. Then branch from the stable foundation.**

---

# 74. Recovery procedure if project context is lost

If all previous planning conversations disappear, use this exact procedure.

## Step A — Read this file

Treat it as the master specification.

## Step B — Inspect repository

Determine:

- current branch;
- latest commit;
- build system;
- source directories;
- CI workflows;
- existing docs;
- current test status.

## Step C — Determine actual implementation phase

Use source and CI evidence, not this plan alone.

## Step D — Run clean build

Record the first failing stage.

## Step E — Run QEMU smoke test

Record the last successful boot checkpoint.

## Step F — Compare against roadmap

Do not skip unfinished prerequisites.

## Step G — Continue from the earliest broken prerequisite

Do not jump to GUI polish if the kernel memory manager is broken.

## Step H — Preserve new discoveries

If a root cause is found, add a concise entry to the relevant architecture/debugging section and add a regression test where possible.

---

# 75. Current-state ledger

This table is intentionally a template. Fill it using real repository/CI evidence.

| Subsystem | Planned | Implemented | Build-tested | Runtime-tested | Stable | Notes |
|---|---:|---:|---:|---:|---:|---|
| Toolchain | yes | | | | | |
| Bootloader | yes | | | | | |
| Early console | yes | | | | | |
| PMM | yes | | | | | |
| VMM | yes | | | | | |
| Heap | yes | | | | | |
| GDT/TSS/IDT | yes | | | | | |
| Exceptions | yes | | | | | |
| IRQ/timer | yes | | | | | |
| Scheduler | yes | | | | | |
| Processes | yes | | | | | |
| Syscalls | yes | | | | | |
| Userspace init | yes | | | | | |
| Storage | yes | | | | | |
| VFS | yes | | | | | |
| Configuration | yes | | | | | |
| Keyboard | yes | | | | | |
| Mouse | yes | | | | | |
| Display | yes | | | | | |
| GUI toolkit | yes | | | | | |
| Compositor | yes | | | | | |
| Desktop shell | yes | | | | | |
| First-run | yes | | | | | |
| Localization | yes | | | | | |
| Settings | yes | | | | | |
| Network | yes | | | | | |
| Terminal | yes | | | | | |
| Package manager | yes | | | | | |
| SB Store | yes | | | | | |
| Drivers | yes | | | | | |
| Security | yes | | | | | |
| Diagnostics | yes | | | | | |
| Recovery | yes | | | | | |
| Updates | yes | | | | | |
| Performance | yes | | | | | |
| Release | yes | | | | | |

---

# 76. Final project rule

When in doubt, choose the implementation that best satisfies all of these simultaneously:

- **lightweight**;
- **fast**;
- **stable**;
- **secure**;
- **understandable**;
- **configurable**;
- **recoverable**;
- **open**;
- **modular**;
- **testable**;
- **maintainable**.

Do not optimize for the shortest code at the expense of the system.
Do not optimize for the smallest ISO at the expense of usability.
Do not optimize for visual polish at the expense of kernel stability.
Do not optimize for feature count at the expense of reliability.

The objective is a complete, coherent desktop OS whose components can be replaced, extended and improved without losing the foundations.

**This document is the project's long-term construction and recovery blueprint.**
