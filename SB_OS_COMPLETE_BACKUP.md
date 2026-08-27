# SuiraBox OS — Complete Project Backup, Master Specification and Recovery Blueprint

> **Document role:** exhaustive project backup and implementation handoff.
>
> This document is intentionally written more like a construction manual, architecture reference, test plan, operational handbook and recovery record combined. It exists so that the project can be reconstructed even if chat history, individual AI context, temporary notes, or one developer's memory is lost.
>
> **Current active product:** SB Desktop.
>
> **Current active scope:** build one coherent GUI desktop OS from the existing x86_64/QEMU foundation. A separate terminal-only product is not an active development target in this phase. SB Desktop nevertheless includes a real terminal/CLI as a required feature.
>
> **Status rule:** this document is a target and recovery specification. It is never proof that a feature exists. Actual status must be established from source, build output, automated tests, CI, and runtime behavior.

---

## 1. Canonical source hierarchy

When two pieces of information disagree, use this order:

1. Actual source code and generated artifacts.
2. Automated test results and CI logs.
3. `SB_OS_DESIGN.md` and this file for intended architecture/requirements.
4. Individual `docs/*` files.
5. Historical conversations/issues.

A design document must never be used to override a failing runtime test.

`SB_OS_DESIGN.md` remains the concise master design. This file is the exhaustive recovery companion and may contain operational detail that does not belong in the shorter master document.

If these documents ever conflict, update both rather than silently choosing one.

---

# 2. Product definition

## 2.1 What SB is

SuiraBox OS is an open-source, general-purpose desktop operating system designed around:

- very small default installation;
- optional components installed on demand;
- detailed but understandable configuration;
- GUI-first normal usage;
- a real CLI/terminal for advanced control;
- strong diagnostics and recovery;
- broad x86_64 compatibility as the first baseline;
- first-class networking;
- benchmark-driven performance work;
- a Minecraft-first optimization strategy without turning the kernel into a Minecraft-specific kernel.

## 2.2 What SB is not

It is not:

- a collection of screenshots pretending to be an OS;
- a single-app launcher disguised as a desktop OS;
- a kernel where GUI behavior is hard-coded into privileged layers;
- an OS that requires terminal commands for normal first boot;
- an OS that shows BSOD/RSOD for normal application errors;
- an excuse to ship every possible service in the base image;
- an excuse to make unsupported hardware appear supported through a label.

## 2.3 User experience goal

Normal user journey:

`Power on → firmware → bootloader → kernel → hardware discovery → memory → core services → graphics/input → desktop → first-run language popup → normal desktop`

Advanced path:

`Desktop → Settings / Terminal / Diagnostics / Recovery / Store`

The user should not be required to understand the implementation details to use the machine.

---

# 3. Product philosophy

## 3.1 Minimal core

The base installation contains only what is necessary for:

- boot;
- security;
- recovery;
- basic hardware discovery;
- storage;
- networking foundations;
- configuration;
- display/input;
- desktop foundation.

Large applications, runtimes, fonts, themes, developer kits and specialized drivers are optional whenever practical.

## 3.2 User choice

Users should be able to:

- add components;
- remove components;
- enable/disable services;
- select defaults;
- switch performance profiles;
- change language;
- change keyboard layout;
- change network configuration;
- inspect resource consumption;
- recover from failed optional components.

Security-critical defaults may remain mandatory, but non-security-critical behavior should be configurable.

## 3.3 Yogibo-like flexibility principle

The system should adapt to the user's desired configuration rather than forcing one fixed operating pattern.

The configuration UI therefore needs three levels:

- Basic: simple common options.
- Advanced: detailed options with explanations.
- Expert: low-level controls with warnings and recovery guidance.

## 3.4 Performance definition

Performance means practical responsiveness, not only a small ISO.

Measure:

- boot duration;
- time to usable desktop;
- idle memory;
- idle CPU;
- background wakeups;
- application launch time;
- GUI frame latency;
- storage latency/throughput;
- network latency/throughput;
- package download and installation cost.

Every SB-specific optimization requires a baseline and a reproducible measurement.

---

# 4. Hard engineering rules

1. Correctness first.
2. Data integrity first.
3. Security is a functional requirement.
4. Never claim a feature is complete from a source-file placeholder.
5. Never claim hardware support without a test.
6. Never weaken a test to hide a failure.
7. Never silently delete user data.
8. Never make a low-level subsystem depend on GUI code merely to report an error.
9. Avoid unbounded loops in boot, parsers, retries and recovery.
10. Avoid busy polling when an event-driven/blocking design is practical.
11. Keep one authoritative source of truth for each configuration value.
12. Keep ownership explicit for persistent files, services, devices and resources.
13. Make configuration changes atomic where practical.
14. Make package operations transactional where practical.
15. Ordinary errors must use ordinary error UX.
16. BSOD is for unsafe/unrecoverable kernel state.
17. RSOD is for exceptional early-boot/output/recovery/integrity conditions.
18. Do not scare users unnecessarily.
19. Serious diagnostics must be technically useful.
20. Sensitive information must be redacted from support reports.
21. GUI and CLI should share underlying services/APIs.
22. Optional software must not unnecessarily consume startup resources.
23. Temporary debug code must be removed or explicitly justified.
24. Every new persistent format needs versioning/migration planning.
25. Every new API needs lifetime, ownership, blocking and failure semantics.
26. Every shared mutable resource needs a synchronization strategy.
27. Every architecture-affecting change must update documentation.
28. No subsystem is stable until runtime behavior is verified.

---

# 5. Complete repository architecture

## 5.1 Current logical ownership

```text
boot/                   earliest boot and CPU handoff
kernel/                 privileged kernel code
userspace/              services, desktop and applications
docs/                   subsystem design and testing notes
site/                   public website
.github/                CI/release automation
Makefile                build orchestration
linker.ld               kernel image layout
README.md               public summary
SB_OS_DESIGN.md         concise master architecture
SB_OS_COMPLETE_BACKUP.md exhaustive recovery blueprint
```

## 5.2 Target kernel organization

```text
kernel/
  arch/x86_64/
    boot/
    cpu/
    interrupt/
    paging/
    timer/
    acpi/
    smp/
  mm/
    pmm/
    vmm/
    heap/
    dma/
  sched/
  process/
  syscall/
  ipc/
  security/
  time/
  power/
  fs/
  net/
  log/
  panic/
  drivers/
    bus/
    pci/
    usb/
    storage/
    display/
    input/
    network/
    audio/
```

## 5.3 Target userspace organization

```text
userspace/
  init/
  services/
    config/
    logging/
    display/
    input/
    network/
    package/
    update/
    account/
    notification/
  gui/
    compositor/
    toolkit/
    desktop/
    settings/
    filemanager/
    terminal/
    store/
  apps/
  runtimes/
```

These are target boundaries. Do not create empty directories just to match a diagram.

---

# 6. End-to-end dependency graph

The intended dependency chain is:

`Firmware/boot environment`

`→ bootloader`

`→ CPU/stack/paging bootstrap`

`→ kernel main`

`→ PMM`

`→ VMM`

`→ heap`

`→ GDT/TSS/IDT`

`→ exceptions/interrupts/timer`

`→ scheduler`

`→ processes/threads`

`→ syscalls/IPC`

`→ userspace init`

`→ block/storage/filesystem/VFS/config`

`→ input/display`

`→ compositor/window system`

`→ desktop shell`

`→ localization/first-run/settings`

`→ network manager`

`→ terminal/package manager/store`

`→ runtime/JVM`

`→ Minecraft tooling`

Higher layers may only assume guarantees explicitly provided by lower layers.

---

# 7. Development phases and exit criteria

## Phase 0 — Build foundation

Deliver:

- compiler/toolchain assumptions;
- assembler configuration;
- linker script;
- Makefile;
- ISO generation;
- QEMU command;
- serial diagnostics;
- CI.

Exit:

- clean checkout builds without undocumented manual steps;
- produced ELF and ISO pass structural checks.

## Phase 1 — Boot

Deliver:

- Multiboot2 entry;
- long-mode transition;
- valid stack;
- preserved boot information;
- early serial output;
- safe image layout.

Exit:

- repeated QEMU boot reaches kernel main without corruption.

## Phase 2 — Memory

Deliver:

- bootstrap PMM;
- full memory-map merge;
- VMM;
- kernel heap;
- page permission enforcement.

Exit:

- allocation/mapping/fault tests pass;
- no allocator reinitialization corrupts live state.

## Phase 3 — CPU runtime

Deliver:

- GDT;
- TSS;
- IDT;
- exception handlers;
- interrupt controller;
- timer;
- synchronization.

Exit:

- controlled exceptions and timer interrupts work repeatedly.

## Phase 4 — Execution

Deliver:

- scheduler;
- threads;
- processes;
- address spaces;
- syscall boundary;
- ELF loader;
- userspace init.

Exit:

- isolated userspace process starts, makes syscalls and exits cleanly.

## Phase 5 — Storage/config

Deliver:

- block abstraction;
- partition detection;
- filesystem;
- VFS;
- persistent configuration;
- recovery-aware writes.

Exit:

- files survive reboot and configuration is atomic/persistent.

## Phase 6 — Desktop hardware

Deliver:

- keyboard;
- mouse;
- framebuffer/display;
- generic device model;
- baseline storage/network devices.

Exit:

- user can interact with a basic graphical environment.

## Phase 7 — GUI

Deliver:

- text/font engine;
- widgets;
- event routing;
- compositor;
- window manager;
- desktop shell;
- notifications;
- clipboard;
- file manager.

Exit:

- interactive desktop can be used without terminal knowledge.

## Phase 8 — Onboarding/localization/settings

Deliver:

- configuration service;
- locale service;
- first-run popup;
- language packs;
- settings UI;
- persistence and migration.

Exit:

- clean install prompts for language graphically and subsequent boot skips it.

## Phase 9 — Networking

Deliver:

- NIC driver interface;
- Ethernet;
- ARP/ND;
- IPv4/IPv6;
- routing;
- UDP/TCP;
- DNS/DHCP;
- sockets;
- network manager;
- firewall policy framework.

Exit:

- userspace can communicate reliably through QEMU and supported baseline hardware.

## Phase 10 — Software distribution

Deliver:

- package format;
- manifest;
- repository metadata;
- resolver;
- downloader;
- verifier;
- transaction engine;
- Store UI.

Exit:

- package install/remove/update operations are safe, recoverable and available from GUI and CLI.

## Phase 11 — Hardening

Deliver:

- accounts;
- permissions;
- sandbox boundaries;
- diagnostics;
- recovery;
- update/rollback;
- cleanup;
- security documentation.

Exit:

- known failure classes have recovery paths and no unresolved release blockers.

## Phase 12 — Release

Deliver:

- complete test matrix;
- compatibility matrix;
- artifact verification;
- release notes;
- installer/recovery docs;
- website publication.

Exit:

- release gates pass.

---

# 8. Boot and firmware specification

## 8.1 Initial environment

Initial development uses x86_64 with QEMU and the existing GRUB Multiboot2 path because it reduces bootloader complexity during kernel bring-up.

A future direct UEFI-oriented boot path can be added after kernel stability.

## 8.2 Boot responsibilities

The early boot path must:

- enter the expected CPU mode;
- establish a known stack;
- preserve required boot information;
- establish required early page tables;
- enter kernel main through a documented ABI.

It must not:

- initialize the GUI;
- start optional services;
- access the package store;
- perform user onboarding.

## 8.3 Reserved memory accounting

Before PMM exposes physical pages, reserve:

- boot code/data still needed;
- kernel image;
- boot stack;
- page tables;
- PMM metadata;
- Multiboot information until no longer needed;
- modules/initramfs until ownership transfers;
- framebuffer memory if applicable;
- ACPI tables while referenced;
- firmware-reserved ranges;
- MMIO regions as appropriate.

## 8.4 Early diagnostics

Early diagnostics must remain independent of heap, filesystem and GUI.

Checkpoint example:

```text
BOOT_ENTRY
CPU_READY
STACK_READY
BOOT_INFO_PRESERVED
PAGING_READY
KERNEL_MAIN
```

---

# 9. PMM exact lifecycle

The PMM must follow a staged lifecycle.

## Stage A — bootstrap allocator

Use only a known-safe bounded region.

Purpose:

- make VMM/heap/early structures possible;
- avoid trusting an unvalidated firmware map too early.

## Stage B — boot-map parsing

Validate the Multiboot/firmware memory information before use:

- structure address within accessible memory;
- total length valid;
- every entry remains inside total length;
- entry size is valid;
- entry count is bounded;
- arithmetic cannot overflow;
- unusable/reserved classes remain reserved.

## Stage C — merge

Do **not** call a resetting PMM initialization function over a live allocator.

Instead merge the new information into the existing state.

Preserve:

- kernel allocations;
- page tables;
- stack;
- PMM bitmap;
- all already allocated pages;
- device-reserved regions;
- boot data still in use.

## Stage D — normal allocator

Expose full discovered memory after reservations are complete.

## Required invariants

- every allocated page is marked allocated;
- every reserved page is unavailable;
- no free page contains active allocator metadata;
- counters match bitmap/list state;
- invalid free is rejected;
- double free is detected where possible;
- operations have bounded work.

## PMM failure diagnosis

Check in order:

1. PMM entry checkpoint;
2. PMM metadata addresses;
3. linker layout;
4. stack bounds;
5. page-table bounds;
6. bitmap bounds;
7. kernel end;
8. boot-info range;
9. loop limits;
10. serial output behavior;
11. second-initialization paths;
12. compiler-generated helper dependencies.

---

# 10. VMM specification

Use the architecture-supported page-table hierarchy.

For x86_64 baseline:

`PML4 → PDPT → PD → PT → page`

Requirements:

- map;
- unmap;
- translate;
- permission management;
- address-space creation/destruction;
- TLB invalidation;
- user/kernel separation.

Permissions must distinguish at least:

- present;
- writable;
- user/supervisor;
- executable/non-executable where supported.

Rules:

- userspace cannot modify page tables directly;
- kernel memory remains supervisor-only;
- invalid accesses produce controlled faults;
- page-table pages come from PMM;
- destroying an address space releases owned resources.

---

# 11. Kernel heap

Requirements:

- documented alignment;
- zero-size behavior;
- overflow checks;
- explicit allocation failure;
- valid free checking where practical;
- fragmentation testing;
- no recursive dependency on itself for early debug logging.

Large allocations may be backed directly by page mappings instead of one monolithic heap arena.

---

# 12. CPU architecture and privilege model

Baseline target is x86_64.

Privilege model:

```text
Ring 0
  Kernel
  Core device drivers
  Scheduler
  Memory manager
  Syscall entry

Ring 3
  Desktop services
  Applications
  Terminal
  JVM
  Minecraft
  Browser
```

Processes require independent user address spaces.

TSS must provide a safe kernel stack for privilege transitions.

Current bring-up syscall transport may use `int 0x80`; the logical syscall ABI must not be tied to that transport so a future `SYSCALL/SYSRET` path can replace it.

---

# 13. GDT/TSS/IDT/exception specification

## GDT

Provide required kernel/user descriptors according to the selected x86_64 segmentation model.

## TSS

Maintain:

- kernel privilege-transition stack;
- later per-CPU state;
- required architectural TSS fields.

## IDT

Install CPU exception handlers before userspace execution.

## Exception context

Capture as available:

- vector;
- CPU error code;
- RIP;
- CS;
- RFLAGS;
- RSP;
- CR2 for relevant faults;
- general registers;
- process/thread ID.

---

# 14. Interrupt controller and timer plan

This section resolves the timer ambiguity identified during audit.

## Bootstrap timer

Use one deterministic legacy-capable timer path first for bring-up, such as PIT when supported by the selected environment.

Purpose:

- verify interrupt delivery;
- provide basic scheduler ticks;
- keep early bring-up simple.

## APIC stage

After basic interrupts work:

1. detect local/APIC capability;
2. initialize interrupt routing;
3. initialize LAPIC timer or selected stable timer source;
4. calibrate against a reliable clock source;
5. switch scheduler timing to the chosen production source;
6. retain fallback if hardware capability is absent.

## Clocksource vs clockevent

Treat these as separate concepts:

- clocksource: high-resolution time measurement;
- clockevent/timer interrupt: event that wakes/schedules work.

Do not assume the scheduler's interrupt source and wall-clock source must be the same device.

## Timer selection rule

The production default should be capability-driven and benchmarked. Candidate hardware includes local APIC timer, HPET where appropriate, and a fallback legacy timer. The exact choice must be encoded in one time subsystem rather than scattered across the scheduler.

## Requirements

- monotonic time;
- no backwards jumps in monotonic clock;
- bounded timer queue operations;
- timer cancellation semantics;
- sleep/wakeup integration;
- per-CPU timers after SMP.

---

# 15. ACPI and SMP plan

## ACPI

Introduce ACPI discovery after the basic memory/interrupt foundation is stable.

ACPI responsibilities may include:

- CPU topology;
- power management tables;
- interrupt routing information;
- battery information;
- platform devices;
- thermal information later.

ACPI tables must remain protected in memory while referenced.

Malformed tables are a diagnostic condition, not permission for arbitrary memory access.

## SMP

Start with a single CPU.

Then:

1. discover CPU topology;
2. initialize additional CPUs;
3. assign per-CPU data;
4. provide per-CPU TSS/interrupt state;
5. introduce per-CPU scheduler queues;
6. synchronize shared kernel structures;
7. test cross-CPU interrupts;
8. test load balancing.

A single-core fallback remains mandatory.

---

# 16. Scheduler and concurrency

## Scheduler

Start with deterministic round-robin/preemptive behavior suitable for bring-up.

Later support:

- priorities;
- CPU affinity;
- per-CPU queues;
- load balancing;
- sleep/wakeup;
- workload hints.

## Concurrency rules

Every shared mutable structure must define:

- lock type;
- lock owner;
- lock ordering;
- interrupt-context restrictions;
- whether sleeping while held is legal;
- lifetime/reference ownership.

## Deadlock prevention

Low-level code must not accidentally use a high-level lock.

Document lock order globally where multiple locks can nest.

Never hold a spinlock across potentially blocking operations.

---

# 17. Processes, threads and userspace

Lifecycle:

`create → initialize → runnable → running → waiting/sleeping → runnable → exit → cleanup`

A process owns:

- address space;
- thread(s);
- handles/file descriptors;
- IPC endpoints;
- accounting information;
- service registrations.

Exit must release resources deterministically.

Userspace faults should normally terminate/isolate the offending process rather than the entire OS.

---

# 18. Syscall ABI

Logical syscall ABI must be versioned independently of the transport mechanism.

Initial known bootstrap calls include timer/process information and exit; later add:

- process/thread control;
- memory mapping;
- files;
- time;
- IPC;
- sockets;
- service/configuration;
- controlled device operations.

Every boundary validates:

- pointer range;
- length;
- integer overflow;
- handle validity;
- permissions;
- object ownership.

For machine-readable CLI operations, output must have a stable format such as JSON where appropriate.

---

# 19. ELF loader

Loading flow:

`VFS → ELF validation → program headers → permission checks → PMM pages → VMM mappings → stack → entry`

Validation:

- ELF magic/class/machine/version;
- valid program-header range;
- `p_filesz ≤ p_memsz`;
- no address arithmetic overflow;
- user address-space bounds;
- entry point inside executable mapping;
- segment permissions mapped correctly.

Reject malformed images instead of attempting recovery by guessing.

ASLR may be introduced later after stable ET_EXEC/ET_DYN loading exists.

---

# 20. Storage architecture

Layers:

`device → block → partition → filesystem → VFS → file API → application`

## Block layer

Must define:

- logical sector size;
- capacity;
- read/write semantics;
- failure result;
- synchronization/blocking behavior.

## Partition layer

Must recognize supported partition schemes without coupling VFS to disk layout.

## Filesystem

A read-only initial filesystem implementation is acceptable for bring-up. Write support must come only after a robust block/cache/VFS foundation exists.

## VFS

Expose common objects:

- path;
- file;
- directory;
- metadata;
- handle;
- permissions.

## Durability

A successful write must define whether it means:

- buffered in memory;
- handed to filesystem;
- handed to block device;
- durable on media.

The API must not leave durability ambiguous.

---

# 21. Persistent configuration and first-boot lifecycle

This section resolves the language/keyboard ordering ambiguity.

## 21.1 Configuration record

Minimum fields:

```text
format_version
first_boot_complete
locale
region
timezone
keyboard_layout
input_method
performance_profile
network_configuration_reference
integrity/check value
```

## 21.2 First screen

After the graphical desktop becomes available:

```text
Welcome to SuiraBox

Language
[ English ▼ ]

[ Continue ]
```

Options:

- 日本語
- English
- 中文
- Español

This is a GUI popup, not a terminal menu.

## 21.3 Keyboard dependency

Immediately after language selection, derive a **recommended** keyboard layout from locale only as a default suggestion. Do not silently assume that language uniquely determines physical keyboard layout.

Recommended flow:

`Language selection → keyboard layout step or clearly optional keyboard confirmation → account creation only after keyboard is known`

For the simplest first boot, the language popup itself may remain one screen and choose a conservative keyboard default, but before the user enters credentials the actual keyboard layout must be confirmable.

The selected keyboard layout is stored independently from language.

## 21.4 Ordering

Minimum required order:

1. graphical environment available;
2. language selection;
3. locale activated;
4. keyboard layout determined/confirmed;
5. optional region/timezone;
6. network setup optional/skip;
7. account setup when account system is ready;
8. persist transaction;
9. desktop normal mode.

## 21.5 Persistence transaction

Use:

`read old → construct new → validate → write temporary → flush/commit as appropriate → atomic replace → verify → mark complete`

If interrupted, boot must find either the old valid record or a recoverable incomplete record.

## 21.6 Corrupt configuration

Do not crash the kernel.

Safe behavior:

- preserve diagnostic copy when possible;
- invalidate bad state;
- use safe defaults;
- show setup again.

Never make corrupted configuration cause an infinite setup/reboot loop.

---

# 22. Localization

Use stable message IDs rather than literal strings embedded throughout code.

Example:

```text
ui.welcome.title
ui.language.label
ui.continue
settings.language
settings.keyboard_layout
error.network.timeout
panic.kernel.page_fault
```

Initial languages:

- Japanese;
- English;
- Chinese;
- Spanish.

Localization must account for:

- variable string length;
- Unicode;
- fonts/glyph fallback;
- wrapping;
- date/time format;
- number format;
- sorting/collation;
- plural rules;
- time zone;
- keyboard layout;
- input methods.

Language packs should be independently installable when the package system is mature.

---

# 23. Input and IME

Input must distinguish:

- physical key events;
- logical key mapping;
- text input;
- composition/IME input.

Japanese/Chinese input should not be implemented as a hard-coded keyboard hack. The architecture needs an input method interface so additional engines can be installed later.

Minimum initial keyboard path:

`device → raw key event → layout mapping → logical key/text event → focused application`

The application should not need to know the physical scan code layout.

---

# 24. Display and graphics

## Baseline

Provide a generic framebuffer path first.

## Acceleration

Use a layered driver interface:

`GPU/Framebuffer Driver → Display Service → Graphics API → Compositor`

Missing vendor acceleration must fall back to a usable basic display whenever possible.

## Requirements

- resolution detection;
- pixel format definition;
- framebuffer ownership;
- synchronized updates;
- clipping;
- double/triple buffering as appropriate;
- scaling/DPI foundation;
- cursor path;
- multiple displays later.

Do not tie the GUI to one GPU vendor.

---

# 25. GUI toolkit and compositor

Minimum widgets:

- window;
- label;
- text field;
- button;
- list;
- select/drop-down;
- checkbox/toggle;
- dialog;
- menu;
- scroll area;
- notification.

Required behaviors:

- focus;
- keyboard navigation;
- pointer input;
- resizing;
- clipping;
- localization-aware sizing;
- accessibility semantics.

Compositor must be event-driven and avoid per-widget busy polling.

---

# 26. Desktop shell

Minimum:

- launcher;
- task/window management;
- status area;
- network status;
- notifications;
- Settings;
- File Manager;
- Terminal;
- power controls.

Optional applications must be launched on demand rather than resident by default.

---

# 27. Settings architecture

Categories:

- System;
- Appearance;
- Display;
- Sound;
- Network;
- Keyboard & Mouse;
- Storage;
- Applications;
- Privacy & Security;
- Accounts;
- Updates;
- Language & Region;
- Performance;
- Developer;
- Recovery;
- Minecraft where applicable.

Three presentation levels:

- Basic;
- Advanced;
- Expert.

A search field must locate settings by label, alias and technical key.

Every setting defines:

- stable key;
- type;
- valid values/range;
- default;
- current value;
- persistence;
- owning subsystem;
- whether reboot is required;
- whether rollback exists;
- resource impact.

Settings affecting boot, security, drivers, storage or network should use transactional application where practical.

---

# 28. Normal error UX

Severity:

```text
INFO
NOTICE
WARNING
ERROR
CRITICAL
RECOVERY
PANIC
```

Normal errors are dialogs/notifications, not OD screens.

A useful error contains:

- title;
- component;
- human-readable explanation;
- impact;
- automatic recovery action;
- recommended action;
- Error ID;
- Details;
- Support Report when appropriate.

Example:

```text
GPU driver restarted

The graphics driver stopped responding and was restarted.
Your other applications are still running.

Error ID: GPU-DRV-0007

[ Details ] [ Open support report ]
```

---

# 29. BSOD and RSOD specification

## BSOD

Meaning:

> The system was running, but the kernel decided continuing was unsafe or logically invalid.

Use only when kernel-level safe continuation is impossible.

Display when possible:

- error ID;
- panic class;
- exception/vector;
- error code;
- RIP;
- CS;
- RFLAGS;
- CR2;
- RSP/register context where useful;
- process/thread;
- subsystem;
- kernel build;
- boot/session ID;
- recovery result;
- diagnostic report ID.

Provide a plain explanation first and technical detail second.

Safe options:

- restart;
- recovery;
- export diagnostics.

Do not tell the user that the system is safe to continue using after a kernel panic.

## RSOD

Meaning:

> The normal output/recovery path itself cannot be trusted, or critical early system state cannot be trusted.

Use for exceptional classes:

- display initialization emergency;
- critical early-boot integrity failure;
- normal recovery renderer unavailable.

RSOD must not appear for ordinary application crashes, network failures, package failures or normal driver recovery.

---

# 30. Low-level logging architecture

This section resolves the logging deadlock concern.

## Log layers

### Level 0 — early console

- fixed-size;
- non-allocating;
- non-blocking where possible;
- no heap dependency;
- no filesystem dependency;
- no general-purpose mutex dependency.

### Level 1 — kernel ring buffer

Use a bounded ring buffer for structured events.

Fields:

- timestamp;
- severity;
- component;
- event ID;
- CPU ID;
- process/thread if known;
- message/parameters.

### Level 2 — persistent logger

After storage is available, flush selected events to persistent storage according to policy.

## Interrupt/panic rule

Interrupt handlers, PMM internals, spinlock paths and panic code must not call a logging function that can:

- allocate heap memory;
- acquire a sleeping mutex;
- perform blocking disk I/O;
- wait indefinitely for another CPU;
- invoke GUI code.

They may write fixed-size records to the low-level ring buffer/emergency channel.

## Lock-order rule

Logging must never acquire locks in an order that inverts subsystem locks.

A diagnostic operation must not be capable of deadlocking the subsystem it is diagnosing.

## Panic logging

Panic path should use preallocated/static storage only.

---

# 31. Package manager and Store concurrency

This section resolves GUI/CLI package locking.

The Store and CLI must use the same transaction engine.

## Global package transaction lock

Only one package database transaction may mutate installed state at a time.

Operations:

`IDLE → LOCKED → RESOLVING → DOWNLOADING → STAGED → COMMITTING → VERIFIED → RELEASED`

Other clients querying metadata may continue if safe, but conflicting mutation must be blocked.

## GUI behavior during CLI transaction

Show:

```text
Software changes are currently in progress.
The Store will refresh when the operation finishes.
```

Do not allow stale GUI state to overwrite the CLI transaction.

## CLI behavior during GUI transaction

The CLI receives a stable error such as:

`SB_E_PACKAGE_BUSY`

It may wait only when explicitly requested.

## Crash recovery

Transaction journal records:

- transaction ID;
- target packages;
- pre-state;
- intended state;
- stage;
- completed steps;
- rollback data.

After reboot, detect incomplete transactions and either:

- resume safely;
- roll back;
- enter package recovery.

Never assume a process crash means no filesystem changes occurred.

---

# 32. Network architecture

Networking is a core system requirement.

Target:

`NIC driver → link → ARP/ND → IPv4/IPv6 → routing → UDP/TCP → DNS/DHCP → sockets → network manager`

Required eventual functions:

- Ethernet;
- Wi-Fi drivers later;
- IPv4;
- IPv6;
- ARP;
- Neighbor Discovery;
- routing;
- DHCP;
- DNS;
- sockets;
- firewall/policy;
- diagnostics.

## Timeout policy

Every network operation defines:

- timeout;
- retry count;
- cancellation;
- failure code.

Network failure must not freeze the GUI.

## Network configuration

GUI and CLI both call the same network service.

Profiles contain:

- interface;
- addressing mode;
- static address if used;
- gateway;
- DNS;
- routes;
- firewall profile.

Passwords/secrets for protected network credentials require dedicated secret handling rather than plain logs.

---

# 33. Driver architecture

Driver lifecycle:

```text
DISCOVERED
 → MATCHED
 → PROBED
 → STARTED
 → RUNNING
 → QUIESCED
 → STOPPED
 → REMOVED
```

Callbacks conceptually:

`probe/start/stop/remove/suspend/resume`

Device metadata:

- bus;
- class;
- vendor/device IDs;
- BDF where PCI applies;
- MMIO/I/O resources;
- IRQ/MSI/MSI-X capability;
- DMA capabilities;
- power state;
- driver state.

## DMA

Reserve an abstraction for:

- buffer allocation;
- map/unmap;
- synchronization/coherency;
- IOMMU.

Drivers must not assume unrestricted physical memory access.

---

# 34. PCI / PCIe

Discovery:

`root complex → bus/device/function enumeration → device model → driver matching`

Read-only enumeration is the first milestone.

Later add:

- BAR resource management;
- MSI/MSI-X;
- bus mastering;
- power management;
- hotplug;
- ACS/SR-IOV where justified;
- IOMMU integration.

BAR accesses must be validated and mapped through the device layer.

---

# 35. USB, HID and hotplug

USB later enables broader:

- keyboard;
- mouse;
- storage;
- network adapters;
- audio;
- other peripherals.

Hotplug state must be event-driven.

Device removal must safely tear down:

- outstanding I/O;
- mappings;
- user handles;
- services;
- notifications.

A disconnected device must not leave dangling pointers in higher layers.

---

# 36. Storage devices

Baseline future drivers:

- QEMU virtual block device;
- AHCI/SATA where justified;
- NVMe;
- VirtIO block where useful.

The generic block API must hide device-specific command queues and DMA implementation.

Storage errors need distinguishable categories:

- temporary;
- timeout;
- media/error condition;
- disconnected device;
- permission/policy;
- filesystem corruption.

---

# 37. Audio

Audio is not required for the first kernel milestone, but the final desktop target needs a stable audio service.

Target layers:

`audio driver → audio service → mixer/session → applications`

Applications must not directly control hardware buffers.

---

# 38. Power management

Eventually support:

- shutdown;
- reboot;
- suspend/sleep where supported;
- wake events;
- battery state;
- thermal/power status.

Shutdown sequence:

`stop user apps → flush writes → stop services → sync storage → disable devices → power transition`

A forced shutdown path must exist for failure cases but must clearly warn that data may not have been persisted.

---

# 39. Accounts, permissions and security

Final desktop requires:

- user identity;
- authentication;
- groups/roles;
- file ownership;
- permissions;
- privileged administration;
- application capability/permission policy.

The goal is not to make every application privileged.

Security requirements:

- kernel/userspace separation;
- memory permission enforcement;
- syscall validation;
- protected configuration;
- package verification;
- update integrity;
- network policy;
- sanitized diagnostics.

---

# 40. Application model

Application manifest should define at least:

- package ID;
- version;
- executable;
- architecture;
- permissions/capabilities;
- services;
- startup policy;
- resources;
- localization;
- icons/assets;
- uninstall behavior.

Applications should use private data directories rather than scattering files across system directories.

Large user files must remain user-owned even if generated by an application.

---

# 41. File manager

Minimum:

- navigate directories;
- create directory;
- open files;
- rename;
- move;
- copy;
- delete;
- metadata;
- search later.

Large operations must be asynchronous.

Delete operations must distinguish:

- user file;
- app-owned file;
- cache;
- system file.

A destructive operation must clearly identify affected data.

---

# 42. Notifications

Notifications are not crash screens.

Types:

- informational;
- success;
- warning;
- error;
- critical.

A notification should be:

- localized;
- concise;
- actionable where possible;
- traceable to an Error/Event ID.

Critical failure must escalate to a dialog/recovery UI when a simple notification is insufficient.

---

# 43. Recovery system

Recovery must exist as a first-class subsystem, not a collection of ad-hoc fixes.

Modes:

- normal recovery;
- safe/reduced graphics mode;
- package rollback;
- configuration reset;
- filesystem check/repair integration;
- boot recovery;
- diagnostics export.

Every recovery action must be:

- bounded;
- logged;
- reversible where practical;
- understandable.

Do not restart a broken service endlessly.

---

# 44. Update system

Target flow:

`metadata → verify → resolve → download → verify artifacts → stage → validate → activate → health-check → finalize`

Use A/B or equivalent transactional activation later where justified.

The old known-good system state should be preserved until the updated state passes health checks.

Configuration migration must be versioned.

Failed migration must have a rollback path.

---

# 45. Backup and restore

The project needs two distinct concepts:

## User backup

Protect user data:

- documents;
- Minecraft worlds;
- screenshots;
- projects;
- application data;
- server data.

## System recovery snapshot

Protect system state:

- base configuration;
- package state;
- boot metadata;
- system files;
- rollback information.

Do not treat caches as irreplaceable backup data.

---

# 46. Data hygiene

Classify files by ownership/state:

```text
SYSTEM_REQUIRED
SYSTEM_REBUILDABLE
USER_DATA
PACKAGE_OWNED
CACHE
TEMP
LOG
RECOVERY
UNKNOWN
```

Cleanup rules:

- Safe: rebuildable data only.
- Recommended: safe + proven package or cache orphans.
- Expert: explicit user-selected paths.

Never delete unknown files automatically.

Never remove recovery data just because it is old without policy/confirmation.

Use ownership metadata, not filename guessing alone.

---

# 47. Performance architecture

## Base OS

Minimize:

- startup services;
- resident applications;
- background polling;
- duplicate caches;
- repeated scans.

## Lazy initialization

Do expensive work only when needed when it does not compromise correctness.

## Parallel initialization

Independent services may initialize concurrently once synchronization and failure isolation exist.

Do not parallelize early boot simply for appearance if ordering correctness is not proven.

## Benchmark requirements

Every optimization PR/commit should record:

- hardware/QEMU configuration;
- workload;
- baseline;
- optimized result;
- variance;
- regression threshold.

---

# 48. Minecraft-first strategy

SB remains a general desktop OS.

Minecraft-specific optimization belongs primarily in:

- policy layers;
- runtime/JVM integration;
- SB Hub;
- performance tools;
- package/runtime configuration;
- filesystem/network policies.

## Planned user-space components

```text
SB Hub
 ├─ Minecraft launcher integration
 ├─ Instance Manager
 ├─ Mod Manager
 ├─ Loader Manager
 ├─ Java Runtime Manager
 ├─ Server Manager
 └─ Performance Monitor
```

## JVM

JVM remains userspace.

The kernel provides:

- threads;
- scheduling;
- memory;
- I/O;
- networking;
- timing;
- synchronization.

The JVM layer may expose:

- runtime selection;
- heap policy;
- thread/CPU hints;
- instance-specific settings;
- diagnostics.

All SB-specific JVM optimizations require benchmarks.

---

# 49. Performance profiles

Suggested profiles:

- Balanced;
- Maximum Gaming;
- Minecraft Performance;
- Battery Saver;
- Low Resource;
- Custom.

Profiles are configuration bundles, not separate kernels.

A profile may control:

- scheduler hints;
- service startup;
- cache retention;
- graphics effects;
- background indexing;
- memory policies;
- network policies;
- application priorities.

Profiles must be exportable and importable as declarative configuration.

---

# 50. Hardware compatibility strategy

Initial baseline:

- x86_64;
- QEMU;
- generic framebuffer;
- baseline PCI/storage/input/network devices.

Future hardware:

- Intel graphics;
- AMD graphics;
- NVIDIA RTX-class GPUs;
- NVIDIA professional/data-center GPUs;
- modern storage controllers;
- common Wi-Fi chipsets;
- USB devices;
- workstation/server CPUs.

Do not hard-code hardware support into the base UX.

Capability detection determines which features are enabled.

Fallback hierarchy:

`preferred driver → generic driver → reduced feature mode → actionable error`

---

# 51. Installation system

The eventual installer must make destructive actions explicit.

It should show:

- target disk;
- partition changes;
- data-loss consequences;
- filesystem choice;
- bootloader destination;
- encryption option where supported;
- timezone;
- keyboard;
- locale;
- user/account information.

Before commit, present a final summary.

Installer operations must be recoverable where practical and must never silently format a disk.

---

# 52. Live/recovery environment

A future live/recovery environment should be able to:

- inspect storage;
- inspect logs;
- export diagnostics;
- recover configuration;
- roll back packages;
- repair supported filesystem metadata;
- reinstall boot components where supported.

It should use as little of the installed system as necessary so a damaged installation can still be recovered.

---

# 53. Support report specification

Report fields:

```text
report_version
report_id
created_at
os_version
build_id
architecture
boot_session
hardware_summary
cpu_summary
memory_summary
storage_summary
display_summary
network_summary
installed_core_components
recent_relevant_events
error_ids
recovery_actions
redactions
```

Never include by default:

- passwords;
- authentication tokens;
- private keys;
- cookies/session data;
- raw credential stores;
- arbitrary user file contents.

The report must state what was redacted.

---

# 54. Security/supply-chain model

Packages and updates need:

- trusted repository metadata;
- package identity validation;
- architecture compatibility;
- dependency validation;
- cryptographic integrity verification;
- official signatures/authenticity where required;
- replay/rollback policy;
- controlled installation paths.

Third-party/community repositories must be visibly distinct from the official repository.

The Store must show:

- publisher;
- trust source;
- permissions;
- size;
- dependencies;
- architecture;
- installed impact.

---

# 55. Package manifest model

Required conceptual fields:

```text
id
name
version
channel
architecture
size
sha256
license
publisher
```

Additional:

```text
description
category
dependencies
conflicts
provides
permissions
services
startup
install_path
update_policy
recovery
variants
```

The manifest is metadata, not the payload.

Clients should be able to inspect metadata without downloading the full package.

---

# 56. Package cache and download engine

Requirements:

- resumable downloads;
- cache reuse;
- compressed metadata;
- compact indexes;
- optional delta updates;
- mirror/CDN selection later;
- bounded parallelism;
- integrity verification.

Do not use parallel downloads if they make the real bottleneck worse.

Never sacrifice verification to improve download speed.

---

# 57. Repository architecture

Repository classes:

- Official;
- Community;
- Local.

Repository metadata should include:

- repository identity;
- maintainer/publisher information;
- supported architectures;
- package index version;
- metadata signature;
- compatibility information.

The client should detect repository changes safely and avoid silently trusting a changed identity.

---

# 58. Terminal/CLI requirements

The GUI edition must contain a real terminal.

Core commands eventually include:

```text
sb search <query>
sb info <package>
sb install <package>
sb remove <package>
sb update
sb list
sb clean [safe|recommended|expert]
sb doctor
sb settings get <key>
sb settings set <key> <value>
sb service list
sb service enable <name>
sb service disable <name>
sb process list
sb repo list
sb repo add <url>
sb recovery list
sb recovery rollback <id>
```

CLI output modes:

- human-readable;
- machine-readable.

Errors need stable codes and recovery hints.

GUI and CLI must share the same services.

---

# 59. Resource governance

Optional applications/services may declare:

- CPU priority;
- memory limit;
- network access;
- GPU access;
- startup policy;
- background permission;
- storage scope.

The system should expose resource impact visibly in Settings/Store.

Do not implement arbitrary resource limits until the underlying enforcement exists.

---

# 60. Accessibility

GUI should support from the beginning:

- keyboard navigation;
- visible focus;
- scalable text;
- high-contrast foundation;
- readable notifications;
- semantic UI tree for future assistive technology.

Do not bake all accessibility information out of reach by encoding the GUI as anonymous pixels only.

---

# 61. Privacy

Potential controls:

- diagnostics sharing;
- telemetry policy;
- application permissions;
- device access;
- network access;
- crash-report contents;
- analytics.

Default behavior must be documented.

Collect no data solely because the implementation finds it convenient.

---

# 62. Search and indexing

Desktop search may be added later.

If added:

- obey permissions;
- respect privacy settings;
- use bounded CPU/storage budgets;
- avoid aggressive mandatory full-disk indexing by default;
- make index storage clear and rebuildable.

---

# 63. Diagnostics and developer mode

Developer mode can expose:

- verbose logs;
- kernel diagnostics;
- tracing;
- debug symbols;
- experimental features.

Developer mode must not silently disable core security protections.

Controlled test hooks for BSOD/RSOD must only be available in explicit developer/test contexts.

---

# 64. CI architecture

Current CI validates build and QEMU smoke behavior. The eventual pipeline must expand to:

1. dependency setup;
2. clean build;
3. static/layout validation;
4. Multiboot validation;
5. ISO generation;
6. QEMU boot;
7. serial log collection;
8. memory tests;
9. userspace tests;
10. storage tests;
11. network tests;
12. GUI startup tests;
13. first-run test;
14. persistence test;
15. package transaction test;
16. recovery test;
17. artifact integrity test.

## Runtime timeout rule

A timeout is a failure, not proof that the operation is merely slow.

Investigate:

- infinite loops;
- blocking I/O;
- page faults;
- memory corruption;
- deadlock;
- bad interrupt state;
- wrong CPU mode;
- address overlap;
- broken test harness.

---

# 65. Testing matrix by subsystem

## Boot

Test:

- clean boot;
- repeated boot;
- different QEMU RAM sizes;
- malformed boot metadata where testable;
- linker layout invariants.

## PMM

Test:

- first allocation;
- repeated allocation;
- exhaustion;
- free/reallocate;
- invalid free;
- double free;
- reserved memory;
- map merge.

## VMM

Test:

- map;
- translate;
- unmap;
- permissions;
- page fault;
- process isolation.

## Scheduler

Test:

- creation;
- scheduling;
- sleep/wakeup;
- fairness;
- preemption;
- multi-CPU later.

## Syscalls

Test:

- valid arguments;
- invalid pointers;
- overflow;
- invalid handles;
- permission violations.

## Filesystem

Test:

- file read/write;
- directories;
- rename;
- delete;
- interrupted writes;
- corruption behavior.

## GUI

Test:

- startup;
- keyboard;
- mouse;
- focus;
- select control;
- dialogs;
- localization;
- notification;
- resize;
- display fallback.

## Package manager

Test:

- dependency resolution;
- integrity failure;
- install;
- remove;
- interrupted transaction;
- concurrent GUI/CLI access;
- rollback.

---

# 66. Fault injection

Before stable release, controlled developer tests should deliberately exercise:

- allocation failure;
- disk read error;
- disk write error;
- corrupt filesystem metadata;
- network timeout;
- network disconnect;
- driver start failure;
- driver restart;
- application crash;
- malformed package;
- failed update;
- corrupt configuration;
- userspace page fault;
- kernel exception.

The expected response must be documented for every injected fault.

---

# 67. Release states

Use exact status vocabulary:

`PLANNED`

No implementation.

`SCAFFOLDED`

Interfaces/basic structure exist.

`IMPLEMENTED`

Code exists and has been exercised to some extent.

`BUILD-VERIFIED`

Build/image tests pass.

`RUNTIME-VERIFIED`

Relevant runtime tests pass.

`STABLE`

Release criteria are satisfied.

`RELEASE-READY`

Feature has passed the release matrix and documentation requirements.

Never collapse these into a generic “done.”

---

# 68. Change management

When an architectural decision changes:

1. identify old behavior;
2. state why it changes;
3. identify dependencies;
4. update this file;
5. update `SB_OS_DESIGN.md`;
6. update relevant subsystem document;
7. add migration/recovery path;
8. update tests;
9. verify old configurations/images do not break unexpectedly.

For persistent formats, maintain migration versions.

---

# 69. AI handoff protocol

Any AI continuing the project must:

1. Read this file.
2. Read `SB_OS_DESIGN.md`.
3. Inspect current tree.
4. Inspect relevant source.
5. Inspect latest CI.
6. Establish actual implementation state.
7. Find earliest broken prerequisite.
8. Change the smallest root-cause unit.
9. Build.
10. Runtime-test.
11. Inspect logs.
12. Add regression test.
13. Update docs.
14. Report exactly what changed.

AI must never:

- invent a file;
- invent test results;
- invent hardware support;
- call a mock complete;
- remove tests to make CI pass;
- silently replace architecture;
- silently remove existing features;
- claim persistent behavior without persistence tests.

---

# 70. Project recovery procedure

If context is lost:

1. Read this file.
2. Read `SB_OS_DESIGN.md`.
3. Inspect `README.md` and repository tree.
4. Inspect CI workflow.
5. Run a clean build.
6. Run QEMU smoke test.
7. Record the last successful checkpoint.
8. Compare actual source against the roadmap.
9. Continue at the earliest failing prerequisite.
10. Do not skip directly to GUI polish.
11. Preserve every newly discovered root cause in the documentation.
12. Add regression coverage.

The recovered engineer must assume that source reality is more trustworthy than old prose.

---

# 71. Current repository consistency requirements

The repository currently contains several overlapping architecture documents. This is useful only if their roles are clear.

Required rule:

- `SB_OS_DESIGN.md` = concise master design and current architectural contract.
- `SB_OS_COMPLETE_BACKUP.md` = exhaustive recovery/implementation document.
- Individual `docs/*.md` = subsystem-specific details.
- `README.md` = public summary, not implementation truth.

When a major architectural decision changes, synchronize the affected documents in the same change set whenever practical.

The README's Minecraft-first language is compatible with the broader desktop goal: SB is general-purpose, with Minecraft as a major optimization target. Minecraft-specific behavior must not contaminate generic kernel interfaces.

---

# 72. Public ecosystem plan

Suiram is the project/community identity.

Public entry points should eventually include:

- official GitHub repository;
- GitHub Pages website;
- GitHub Releases;
- documentation;
- official Discord;
- official X account;
- support/troubleshooting material.

The website must be the canonical directory for official links so users can distinguish official from community-created services.

Public release pages should show:

- version;
- edition;
- architecture;
- exact file name;
- file size;
- checksum;
- signature information where used;
- supported hardware;
- known issues;
- installation instructions;
- recovery instructions.

---

# 73. Licensing and third-party components

Before the first public stable release, the repository must include:

- chosen OS license;
- third-party notices;
- contribution terms;
- security policy;
- code of conduct;
- support policy.

Any dependency or asset must have its license reviewed before redistribution.

Minecraft, Java/JVM components, logos, names, libraries and other third-party assets must be described accurately and must not imply unsupported endorsement or ownership.

---

# 74. Release artifact strategy

Initial philosophy:

One reliable generic x86_64 desktop image first.

Later, when real compatibility evidence justifies it, artifact variants may include:

- generic;
- NVIDIA-focused;
- AMD-focused;
- workstation;
- gaming;
- server-oriented;
- high-end accelerator profiles.

Variants should share as much common source and package metadata as practical.

Do not create a dozen images merely because it is technically possible.

---

# 75. Final end-to-end definition of done

SB Desktop v1 is complete only when a normal user can:

1. boot a supported x86_64 machine or QEMU;
2. reach a graphical desktop without terminal setup;
3. see the language selector on clean first boot;
4. select Japanese, English, Chinese or Spanish;
5. confirm/establish a usable keyboard layout;
6. complete initial setup without command-line knowledge;
7. reboot and retain language/configuration;
8. use keyboard and mouse;
9. use display output reliably;
10. configure networking graphically;
11. use a real terminal;
12. manage files;
13. install optional software through SB Store;
14. perform equivalent package operations through CLI;
15. remove optional software safely;
16. change detailed settings;
17. receive normal understandable error notifications;
18. inspect technical details for serious failures;
19. export sanitized support diagnostics;
20. recover from supported package/configuration/service failures;
21. update safely;
22. roll back supported failed updates;
23. operate without unnecessary optional components consuming base resources.

---

# 76. Final engineering philosophy

When choosing between two technically valid implementations, prefer the one that maximizes the combination of:

- simplicity;
- correctness;
- safety;
- performance;
- user control;
- recoverability;
- observability;
- modularity;
- maintainability.

The project should not pursue “smallest possible code” at the expense of a fragile system.

It should not pursue “most features” at the expense of a reliable foundation.

It should not pursue “fastest benchmark number” at the expense of real usability.

It should not pursue “perfect hardware support” by making the base installation heavy for users who do not need those drivers.

The intended long-term result is a desktop operating system whose core is small, whose optional capabilities are modular, whose failures are understandable, whose diagnostics are useful, whose network/storage layers are serious, and whose performance work is measurable.

**Finish one coherent SB Desktop system first. Expand compatibility and specialization only after the reference platform is genuinely stable.**
