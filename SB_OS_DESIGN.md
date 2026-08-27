# SuiraBox OS — Complete Build Specification

> Canonical implementation specification for SB Desktop. Read this before making architectural changes.
> This document describes the target from early boot through a releasable GUI desktop. It is intentionally explicit so an unfamiliar developer or AI can determine what to build, in what order, why it exists, and how to verify it.

## 0. Product scope

**Current and only active product:** SB Desktop, a lightweight, high-performance, open-source general-purpose x86_64 GUI operating system.

Do not spend current development effort on a separate CUI-only edition. A real terminal is nevertheless a mandatory component of SB Desktop.

The intended user journey is:

`Power on → bootloader → kernel → hardware discovery → memory → kernel services → userspace → display/input → compositor → desktop → first-run language popup → usable desktop`

The base installation must be small. Optional applications, language packs, themes, drivers, development tools and other non-essential components should be installable later rather than consuming base resources.

---

# 1. Non-negotiable engineering principles

1. Correctness and security outrank tiny byte savings.
2. Do not implement a feature as a fake UI and call it complete.
3. Do not claim support without a reproducible test.
4. Keep one source of truth for each configuration value.
5. Keep lower layers independent of higher layers.
6. Prefer component recovery over whole-system shutdown.
7. Normal errors are explanatory and recoverable; BSOD/RSOD are exceptional.
8. Do not silently delete user data.
9. Avoid unnecessary boot-time services and polling.
10. Network functionality is core infrastructure.
11. The terminal is first-class inside the GUI edition.
12. All user-visible strings must be localizable.
13. Initial languages: Japanese, English, Chinese, Spanish.
14. First graphical boot uses a GUI select box for language; it must not require terminal commands.
15. Every important change must be build-tested and runtime-tested where applicable.
16. Never weaken a test merely to make CI pass.
17. Temporary diagnostics must be removed or deliberately documented.
18. If implementation is blocked, record the blocker instead of fabricating completion.

---

# 2. Repository architecture

Expected ownership:

- `boot/` — boot entry, early CPU state, boot protocol handling.
- `kernel/` — privileged kernel code and core subsystems.
- `userspace/` — services, shell, GUI and applications.
- `docs/` — specifications, architecture notes and test plans.
- `site/` — public project website.
- `.github/` — CI and release automation.
- `Makefile` — build orchestration.
- `linker.ld` — kernel image layout.

Dependency direction:

`boot → kernel primitives → kernel services → userspace services → GUI/applications`

Never solve a lower-layer problem by importing a higher-layer dependency.

---

# 3. Master implementation order

Do the following in order. A phase is complete only when its implementation works and has an appropriate test.

1. Toolchain and reproducible build
2. Boot protocol / Multiboot2 entry
3. CPU mode and early stack
4. Early serial/debug console
5. Linker layout and reserved-memory accounting
6. PMM
7. VMM
8. Kernel heap
9. GDT/TSS/IDT
10. Exception handlers
11. PIC/APIC and timers
12. IRQ synchronization primitives
13. Scheduler
14. Threads and processes
15. Syscalls and userspace ABI
16. Userspace init/service manager
17. Block-device abstraction
18. Partition/filesystem/VFS layer
19. Persistent configuration
20. Keyboard/mouse input
21. Framebuffer/display abstraction
22. Font/text rendering
23. GUI primitives
24. Compositor
25. Window manager
26. Desktop shell
27. First-boot language popup
28. Localization infrastructure
29. Settings application
30. Network stack
31. Network manager and GUI
32. Terminal/shell
33. Package manager
34. SB Store/repository client
35. Driver framework expansion
36. Logging/diagnostics/support reports
37. Recovery system
38. Update/rollback system
39. Performance and data-cleanup hardening
40. Release engineering and compatibility matrix

Do not jump to store polish while the kernel cannot reliably boot.

---

# 4. Toolchain, build and CI

## 4.1 Build contract

The build must:

- use the intended freestanding compiler/assembler configuration;
- avoid accidental host libc/runtime dependencies;
- assemble boot code for the intended architecture;
- link with the project linker script;
- verify section and address layout;
- produce the boot image/ISO;
- boot it under QEMU.

All flags that affect ABI, architecture or memory model must be explicit.

## 4.2 CI stages

At minimum:

1. dependency/toolchain setup
2. compilation
3. assembly
4. link
5. image/ISO creation
6. boot-protocol validation
7. QEMU boot
8. serial output capture
9. kernel smoke tests
10. memory tests
11. userspace tests
12. GUI startup test
13. first-run setup test
14. configuration persistence test
15. storage test
16. network test
17. package test
18. recovery test
19. artifact checksum/integrity test

A runtime timeout is a failure until the actual cause is understood.

---

# 5. Boot and early CPU initialization

## Required result

After boot, the kernel must have a known CPU state, known stack, preserved boot information and valid initial mappings.

## Required responsibilities

- establish intended CPU mode;
- establish stack;
- preserve boot protocol information;
- initialize minimum page tables if required;
- transfer control through a documented ABI;
- avoid corrupting kernel, stack, page-table or boot data.

## Reserved memory

Before PMM exposes memory as free, explicitly reserve:

- bootloader structures still needed;
- kernel image;
- stack;
- page tables;
- PMM metadata/bitmap;
- framebuffer if reserved by platform;
- firmware-reserved regions reported by the boot environment.

Never assume a reported RAM range is entirely usable.

---

# 6. PMM — Physical Memory Manager

## Purpose

Provide safe page-level physical memory allocation.

## Interface requirements

Define explicit operations equivalent to:

- initialize;
- reserve range;
- release usable range;
- allocate page;
- allocate contiguous pages when needed;
- free page;
- query statistics.

## Invariants

- page size is centralized and constant for the supported architecture;
- reserved pages cannot be allocated;
- freed pages become allocatable exactly once;
- double free is rejected or detected;
- invalid addresses are rejected;
- metadata cannot overlap allocatable memory;
- allocator initialization cannot erase active state.

## Bootstrap

Use a known-safe bootstrap arena first if the full Multiboot map cannot yet be trusted. Later merge the complete memory map without resetting allocations that are already live.

## Required tests

- first allocation;
- many allocations;
- free/reallocate;
- exhaustion;
- invalid address;
- double free;
- reserved-region protection;
- small-RAM QEMU;
- larger-RAM QEMU.

---

# 7. VMM — Virtual Memory Manager

Provide:

- page-table creation;
- mapping;
- unmapping;
- permission flags;
- address-space creation/destruction;
- TLB management where required.

Rules:

- kernel memory is protected from ordinary user mappings;
- user mappings are validated;
- alignment is checked;
- unmapped access becomes a controlled page fault;
- page-table pages are owned by PMM;
- destroyed mappings do not leak physical pages.

Keep architecture-specific code behind an architecture boundary.

---

# 8. Kernel heap

Provide a kernel dynamic allocator backed by PMM/VMM.

Define:

- alignment;
- zero-size behavior;
- overflow behavior;
- allocation failure;
- free semantics.

Test fragmentation, repeated allocation/free, large allocation and failure paths.

Do not implement logging in a way that recursively depends on the heap being debugged.

---

# 9. GDT/TSS/IDT and exceptions

Implement in this order:

1. GDT
2. TSS
3. IDT
4. exception stubs
5. exception dispatch
6. IRQ routing

Handlers must capture enough state for diagnosis:

- vector;
- error code when supplied;
- instruction pointer;
- stack pointer;
- relevant registers;
- current process/thread when available.

A userspace fault should be isolated to its process when safe. A kernel fault that cannot safely recover enters the panic path.

---

# 10. Interrupts, timers and synchronization

Provide:

- interrupt controller abstraction;
- periodic/high-resolution timer as appropriate;
- interrupt-safe locks;
- wait queues/event primitives;
- sleep/timer facilities.

Never implement normal waiting with an unbounded busy loop.

---

# 11. Scheduler, threads and processes

Required capabilities:

- thread creation/destruction;
- context switching;
- runnable queues;
- sleeping/wakeup;
- process address spaces;
- priorities where justified;
- process exit/wait;
- resource cleanup.

The scheduler must not starve system-critical work or freeze GUI event processing.

---

# 12. Syscalls and userspace ABI

Define a stable, versioned syscall ABI.

Families:

- process/thread;
- virtual memory;
- file I/O;
- time;
- IPC;
- networking;
- configuration/service access;
- controlled device access.

At every syscall boundary validate:

- pointers;
- lengths;
- handles;
- object ownership;
- permissions;
- integer overflow.

GUI, shell and services use the same userspace API rather than privileged shortcuts.

---

# 13. Userspace init and services

Start a minimal init/service manager.

Services should declare dependencies and start only when needed.

Do not make optional applications resident at boot.

Failure of one ordinary service should not automatically panic the kernel.

---

# 14. Storage and filesystem

Layers:

`block device → partition → filesystem driver → VFS/path API → configuration/package/user data`

Keep physical disk details out of the high-level file API.

Required file operations:

- open/create;
- read/write;
- seek;
- close;
- rename;
- delete;
- directory enumeration;
- metadata;
- permissions;
- durable flush where requested.

Separate storage classes:

- base OS;
- user data;
- configuration;
- package database;
- cache;
- temporary;
- logs;
- recovery.

Configuration changes must use atomic replacement or a journal/transaction mechanism where feasible.

---

# 15. Persistent configuration

Create one configuration service/API used by first-run, Settings, network manager, language system and other clients.

Each setting has:

- stable key;
- type;
- default;
- validation;
- scope;
- owner subsystem;
- persistence policy.

Corrupt configuration must have a safe recovery path.

Never create duplicate independent settings databases for the same option.

---

# 16. Input subsystem

Abstract hardware into events such as:

- key press/release;
- pointer movement;
- pointer button;
- wheel;
- future touch events.

Provide input focus and event routing.

Keyboard layout is userspace configuration, not hard-coded kernel behavior.

---

# 17. Display subsystem

First provide a generic framebuffer path so a basic desktop can work without a vendor-specific accelerated GPU driver.

Then add a driver interface for accelerated graphics.

Display layers:

`GPU/framebuffer driver → display service → compositor`

A missing accelerated driver should degrade gracefully when a framebuffer is available.

---

# 18. GUI toolkit

Minimum primitives:

- surface/window;
- text;
- font;
- label;
- button;
- text field;
- list;
- select/drop-down;
- checkbox/toggle;
- dialog;
- menu;
- notification;
- scroll container.

Required behavior:

- keyboard navigation;
- pointer navigation;
- focus;
- resize;
- clipping;
- localization-aware text sizing;
- accessibility foundation.

Applications must not access GPU registers or display hardware directly.

---

# 19. Compositor, window manager and desktop shell

Architecture:

`applications → window system → compositor → display service`

Desktop shell minimum:

- launcher;
- task/window management;
- status area;
- notifications;
- settings;
- file manager;
- terminal;
- network status;
- power/restart/shutdown.

Keep optional applications lazy-loaded.

---

# 20. First graphical boot — exact requirement

This is a mandatory UX requirement.

The final GUI boot sequence is:

`kernel → services → display → compositor → desktop shell → first-run check`

If no valid user configuration exists, show a centered lightweight popup over the desktop:

```text
Welcome to SuiraBox

Language
[ English ▼ ]

[ Continue ]
```

The language control is a **select/drop-down box**.

Initial choices:

- 日本語
- English
- 中文
- Español

When Continue is activated:

1. validate the selected locale;
2. write configuration atomically;
3. activate the localization catalog;
4. mark first-run complete;
5. continue into the desktop.

On later boots, load the saved locale and skip the popup.

If configuration is missing/corrupt, recover safely into first-run/defaults. Never loop forever.

The first-run UI must not require a terminal.

Optional later first-run pages may configure:

- region/time zone;
- keyboard layout;
- network;
- privacy defaults;
- performance profile.

Keep them lightweight and skippable where reasonable.

---

# 21. Localization / i18n / l10n

Use stable message IDs, for example:

- `ui.welcome.title`
- `settings.language`
- `error.network.timeout`
- `panic.kernel.page_fault`

Do not scatter language-specific literals through core logic.

Initial languages:

1. Japanese
2. English
3. Chinese
4. Spanish

The localization layer must support:

- fallback language;
- Unicode text;
- variable-length strings;
- plural/format rules as needed;
- independently installable language packs where practical.

All major GUI, Settings, notifications, recovery and error screens use the same localization system.

---

# 22. Settings application

Categories:

- System
- Appearance
- Display
- Sound
- Network
- Keyboard & Mouse
- Storage
- Applications
- Privacy & Security
- Accounts
- Updates
- Language & Region
- Performance
- Developer
- Recovery

Every page must expose clear descriptions without hiding advanced controls.

Changes should be validated before commit and provide a meaningful error if rejected.

---

# 23. Network architecture

Networking is a core subsystem.

Target stack:

`NIC driver → link layer → ARP/ND → IPv4/IPv6 → routing → UDP/TCP → DNS/DHCP → sockets → network manager`

Eventually support:

- Ethernet;
- Wi-Fi through supported drivers;
- IPv4;
- IPv6;
- DHCP;
- static addressing;
- DNS;
- routing;
- loopback;
- sockets;
- firewall/security policy;
- diagnostic commands.

GUI configuration and terminal commands must use the same network service/API.

All network operations have explicit timeout/error behavior and must not block the desktop indefinitely.

---

# 24. Terminal and shell

SB Desktop includes a real terminal.

It must eventually support:

- command execution;
- environment variables;
- pipes/redirection;
- filesystem operations;
- process inspection/control;
- network tools;
- package management;
- diagnostics;
- recovery commands;
- scripting.

The terminal is an advanced interface. It is never the required first-run setup path.

---

# 25. Package manager

Architecture:

`SB Core → package manager → repository metadata → download → verification → transaction → installed state`

Required:

- package identity;
- versions;
- dependencies;
- conflicts;
- install;
- remove;
- upgrade;
- integrity verification;
- official signatures;
- rollback where feasible;
- transaction log;
- orphan/cache management.

GUI Store and terminal package commands call the same package engine.

---

# 26. SB Store

Optional components may include:

- applications;
- language packs;
- fonts;
- themes;
- development tools;
- multimedia components;
- network utilities;
- desktop extensions;
- legally distributable drivers/firmware.

Download efficiency:

- compact metadata;
- compression;
- local caching;
- resumable transfers;
- parallel transfers when beneficial;
- mirrors/CDN later.

Always verify integrity before activation.

The base OS must remain usable without installing a large catalog of optional software.

---

# 27. Hardware and driver architecture

Initial target: broad generic x86_64 PC compatibility.

Driver priorities:

1. framebuffer/display
2. keyboard
3. mouse
4. PCI
5. storage
6. Ethernet
7. USB needed for basic desktop use
8. audio
9. Wi-Fi
10. accelerated graphics

Future GPU targets may include Intel, AMD and NVIDIA consumer/professional hardware, including high-end RTX generations and data-center families, but support must be implemented and tested rather than assumed.

Use capability detection and fallback modes.

Do not make a single GPU vendor a requirement for basic boot when a generic display path is available.

---

# 28. Error UX

Normal severity progression:

`INFO → NOTICE → WARNING → ERROR → CRITICAL → RECOVERY`

Normal errors must tell the user:

- what happened;
- which component was affected;
- whether the OS remains usable;
- what automatic recovery was attempted;
- what the user can do;
- stable Error ID;
- Details;
- Support Report when appropriate.

Example principle:

A recoverable application or driver failure should isolate/restart the affected component rather than immediately stopping the entire OS.

---

# 29. BSOD and RSOD

## BSOD

Meaning:

> The system was operating, but the kernel determined that continuing was unsafe or impossible and stopped.

BSOD is a last-resort kernel diagnostic mechanism.

Display, where available:

- Error ID;
- failure class;
- exception/vector;
- CPU error code;
- instruction pointer;
- register context;
- process/thread;
- subsystem;
- kernel build;
- boot/session ID;
- recovery result;
- diagnostic report ID.

## RSOD

Meaning:

> The normal display/recovery path itself is compromised, or required early-boot/system data cannot be trusted.

Use only for severe early-boot, display-output, integrity or recovery-path failures.

Do not use BSOD/RSOD for ordinary application errors.

---

# 30. Logging and support diagnostics

Use structured logs where practical.

Every significant event should include:

- timestamp;
- severity;
- component;
- Error ID;
- boot/session ID.

Support Report must be available through GUI Settings/Recovery.

It should include enough technical information for a user, developer or technician to diagnose the problem while excluding:

- passwords;
- authentication tokens;
- private keys;
- browser/session credentials;
- equivalent secrets.

Reports should be exportable for support.

---

# 31. Recovery

Prefer recovery in this order:

1. restart failed application;
2. restart failed userspace service;
3. restart network service;
4. restart display component if safe;
5. rollback failed package/update;
6. boot recovery mode;
7. kernel panic only when safe continuation is impossible.

Every recovery path needs a timeout/failure boundary.

---

# 32. Updates

Target transaction:

`metadata → signature verification → dependency resolution → download → staging → validation → atomic activation → rollback path`

Requirements:

- signed metadata;
- integrity checks;
- dependency-aware updates;
- interrupted-update recovery;
- rollback;
- clear progress UI;
- offline/recovery path.

Never intentionally leave the base system half-updated.

---

# 33. Data minimization and cleanup

Classify every file before cleanup:

- user data;
- system;
- configuration;
- package state;
- cache;
- temporary;
- log;
- recovery;
- unknown.

Safe cleanup targets may include expired temporary data and obsolete package caches.

Never automatically delete unknown or user data merely because it appears unused.

The goal is low disk usage without compromising recoverability or safety.

---

# 34. Performance

Measure:

- boot time;
- idle RAM;
- idle CPU;
- base disk footprint;
- application launch time;
- GUI latency;
- network overhead.

Prefer:

- lazy initialization;
- event-driven services;
- bounded startup work;
- minimal resident processes;
- efficient allocators;
- efficient I/O;
- minimal duplicate caches.

Do not perform unmeasured micro-optimizations that reduce reliability.

---

# 35. Security

Security features require actual enforcement.

Required direction:

- kernel/userspace separation;
- permissions;
- memory protection;
- syscall validation;
- package authenticity;
- secure configuration;
- firewall/network policy;
- least privilege;
- secure updates;
- sanitized diagnostic reports.

Never treat a security option as implemented merely because a checkbox exists.

---

# 36. Compatibility and releases

First stable target:

**Generic x86_64 PC**

Use one broadly compatible release until real hardware/test data justifies specialized images.

Future compatibility profiles may include:

- generic desktop/laptop;
- gaming;
- workstation;
- high-end GPU;
- professional GPU;
- server hardware.

These are later branches of the same stable foundation.

---

# 37. End-to-end release test

Before a major release, test from a clean environment:

1. cold boot;
2. reboot;
3. shutdown;
4. graphical desktop startup;
5. first-run language selector;
6. all four initial languages;
7. language persistence after reboot;
8. keyboard/mouse;
9. display;
10. settings persistence;
11. Ethernet/network setup;
12. terminal command execution;
13. filesystem operations;
14. package installation;
15. package removal;
16. recoverable application failure;
17. support report creation;
18. recovery path;
19. controlled kernel exception/panic test;
20. ISO integrity;
21. QEMU boot;
22. known hardware compatibility checks.

---

# 38. AI/developer operating procedure

Every implementation task follows:

1. Read this document.
2. Inspect the current repository and relevant code.
3. Determine the exact current state.
4. Identify the smallest sound architectural change.
5. Implement it.
6. Compile.
7. Run the relevant test.
8. Inspect logs/results.
9. Fix the root cause if it fails.
10. Add or update tests.
11. Update this specification or subsystem documentation when architecture changes.
12. Record what is actually complete.

Status vocabulary:

- **Implemented:** code exists and relevant tests pass.
- **Partial:** some required behavior exists but the full contract is not complete.
- **Planned:** design exists but implementation has not begun.
- **Blocked:** implementation cannot proceed until a named dependency/problem is resolved.
- **Experimental:** intentionally unstable or exploratory.

Never confuse these states.

Never:

- invent implementation results;
- claim a CI run passed without checking;
- claim hardware support without a test;
- weaken a failing test to hide a defect;
- silently remove functionality;
- add unnecessary dependencies;
- place normal GUI setup in the debug terminal;
- create a mock and call it production;
- erase user data as a shortcut;
- bypass security for convenience.

---

# 39. Definition of Done — SB Desktop v1

SB Desktop v1 is complete only when a normal user can:

1. boot supported x86_64 hardware or QEMU;
2. reach a real graphical desktop;
3. perform first setup without a terminal;
4. choose Japanese, English, Chinese or Spanish from the GUI select box;
5. reboot and retain the language;
6. use keyboard and mouse;
7. configure networking;
8. use a real terminal;
9. manage files;
10. install/remove optional software;
11. change detailed system settings;
12. receive understandable recoverable errors;
13. obtain detailed technical support information after serious failures;
14. recover from supported application/service/package failures;
15. update the system safely;
16. operate with a small base installation and optional components loaded only when needed.

Only after this is genuinely working should the project focus primarily on expanding hardware coverage, specialized profiles and further optimization.
