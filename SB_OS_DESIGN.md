# SuiraBox OS — Master Recovery, Construction, Architecture & Operations Specification

> **Document class:** Canonical master specification / recovery manual / implementation blueprint
>
> **Purpose:** This document is the long-term backup of the entire SB Desktop project. It is intentionally more detailed than a normal design document. If chat history, planning notes, an AI session, a developer, or a portion of the repository is lost, this document must preserve enough information to reconstruct the intended product and the reasoning behind it.
>
> **Product currently being built:** **SB Desktop**
>
> **Current scope rule:** Do not spend active development effort designing a separate CUI-only edition. SB Desktop still requires a complete terminal and CLI because command-line operation is an important capability.
>
> **Critical distinction:** This document describes the intended architecture and acceptance criteria. It does **not** prove that an item is implemented. Implementation status must be determined from source code, tests, CI and real runtime behavior.

---

# 0. How this document is to be used

This file has four simultaneous roles:

1. **Product constitution** — preserves the reason SB exists and what kind of OS it should become.
2. **Engineering specification** — defines subsystem responsibilities, interfaces, invariants and implementation order.
3. **Recovery manual** — provides a path back to the intended architecture if development context is lost.
4. **AI handoff document** — gives another AI enough context to continue work without inventing architecture.

## 0.1 Mandatory procedure before changing code

Every developer or AI must:

1. Read this document.
2. Inspect the repository tree.
3. Inspect the current implementation of the subsystem being changed.
4. Inspect the latest CI result relevant to that subsystem.
5. Determine what is actually implemented, not what the documentation says should exist.
6. Identify the smallest unfinished prerequisite.
7. Make the smallest technically justified change that advances the project.
8. Build it.
9. Run the relevant runtime test.
10. Inspect logs rather than assuming success.
11. Add or update tests.
12. Update documentation if architecture changed.
13. Record the real state.

Never treat a planned feature as an implemented feature.

## 0.2 Source-of-truth hierarchy

When information conflicts, use this priority:

1. **Actual source code and build artifacts** for what exists.
2. **Automated tests and CI logs** for what is verified.
3. **This master specification** for intended architecture and product requirements.
4. Other documents, issue comments and conversations for historical context.

A CI failure cannot be overridden by a sentence in this document.

---

# 1. Product identity

## 1.1 Name

Project: **SuiraBox OS**

Short name: **SB**

Current desktop product: **SB Desktop**

Community/project identity: **Suiram**

The project is intended to be open-source, modular, lightweight, configurable, user-oriented and technically understandable.

## 1.2 Product philosophy

SB should feel more like a flexible toolkit than a monolithic operating system that forces one workflow on every user.

Core ideas:

- Minimal base installation.
- Optional functionality installed on demand.
- Detailed but understandable settings.
- GUI-first ordinary usage.
- Real terminal retained for advanced users.
- Fast installation of optional components.
- Strong diagnostics.
- Recoverable failures whenever possible.
- No unnecessary background work.
- No intentional accumulation of obsolete data.
- Hardware support through clean abstractions.
- Open architecture that can later support many hardware classes.
- User choice without making the system incomprehensible.

## 1.3 The “RPG toolkit” principle

The intended experience is conceptually similar to a system that lets the user assemble the capabilities they need:

`Small base → choose components → install → configure → use`

This does **not** mean every library must be a separate package. Excessive fragmentation creates dependency, update and maintenance problems. Components should be split when the split gives a real benefit in installation size, memory use, security, maintainability or user choice.

## 1.4 Performance principle

“Lightweight” is a measured property, not a marketing word.

Track at least:

- ISO/base image size;
- installed base footprint;
- boot time;
- idle RAM;
- idle CPU;
- background wakeups;
- GUI latency;
- application startup time;
- storage I/O;
- package download size;
- package installation time;
- network overhead;
- update cost.

Never introduce a correctness or security regression merely to save a few bytes.

---

# 2. Non-negotiable engineering rules

1. Never claim code is implemented when it is only planned.
2. Never claim hardware support without a reproducible test.
3. Never call a visual mockup a functional desktop feature.
4. Never weaken a test solely to make CI pass.
5. Never remove diagnostics just because they make a failure look bad.
6. Never silently delete user data.
7. Never allow a lower architectural layer to depend on a higher layer for convenience.
8. Keep one source of truth for important configuration values.
9. Prefer bounded loops and bounded waits.
10. Avoid busy polling when an event-driven or blocking design is possible.
11. Normal application errors remain normal application errors.
12. Only use kernel panic when safe continuation is impossible.
13. RSOD is reserved for extremely severe early-boot/display/recovery/integrity conditions.
14. BSOD is reserved for unsafe or unrecoverable system state.
15. Every user-visible string must be localizable.
16. Initial GUI languages: Japanese, English, Chinese and Spanish.
17. First-run language setup is graphical.
18. First-run language setup uses a select/drop-down control.
19. The terminal remains a required part of SB Desktop.
20. Network operations must have explicit timeout and failure behavior.
21. Optional software should not unnecessarily consume boot resources.
22. Persistent configuration should use atomic/transactional writes where practical.
23. Security must be enforced at privileged boundaries, not only in GUI code.
24. Diagnostic reports must exclude secrets.
25. Temporary debugging code must be removed or explicitly justified.
26. Every architectural change needs a reason.
27. Every major subsystem needs a test strategy.
28. Runtime behavior determines status, not file existence.
29. Backward compatibility must be considered before changing persistent formats or public APIs.
30. Data corruption is treated as a higher-severity failure than a feature being temporarily unavailable.
31. Recovery paths must not create infinite recovery loops.
32. A user must be able to understand serious errors without knowing kernel terminology.
33. Advanced diagnostics must still contain enough technical information for a developer or repair/support process.
34. Privacy-sensitive information must never be collected merely because it is technically convenient.

---

# 3. Target user experience

## 3.1 Normal cold boot

Target flow:

`Firmware → bootloader → kernel → hardware discovery → memory → interrupts → scheduler → userspace init → storage/config → display/input → compositor → desktop → first-run check → desktop ready`

## 3.2 First boot

A clean installation with no valid language configuration must eventually reach the desktop and then show a small centered popup:

```text
Welcome to SuiraBox

Language
[ English ▼ ]

[ Continue ]
```

Choices:

- 日本語
- English
- 中文
- Español

The user must not be required to type terminal commands.

After selection:

1. Validate the selection.
2. Write configuration atomically.
3. Activate the localization service.
4. Refresh the current UI if necessary.
5. Mark first-run as completed.
6. Continue into normal desktop operation.

If the configuration is missing or corrupted later, the system must safely recover to the setup flow instead of silently using invalid state or entering a boot loop.

## 3.3 Normal error

Normal errors should be non-traumatic and recoverable whenever possible.

A normal dialog should explain:

- what happened;
- which component was affected;
- whether the system is still usable;
- whether recovery was attempted;
- what the user can do next;
- Error ID;
- optional technical details;
- support-report option where appropriate.

## 3.4 BSOD

Meaning:

> “The system was operating, but continuing is no longer safe.”

A BSOD is a kernel/system emergency, not a normal error dialog.

Display, where available:

- stable error identifier;
- failure class;
- CPU exception/vector;
- hardware error code;
- instruction pointer;
- relevant register state;
- current process/thread;
- subsystem;
- kernel version/build;
- boot/session identifier;
- whether a crash dump was saved;
- recovery/reboot state;
- support-report identifier.

The screen should be understandable to ordinary users while offering an expandable technical section.

## 3.5 RSOD

Meaning:

> “The normal display/recovery path itself cannot be trusted, or critical early-boot/system integrity is broken.”

RSOD must be rare. It is not a second ordinary error color.

Use it only when:

- the normal graphics path is itself compromised;
- early boot state cannot be trusted;
- recovery/display initialization is critically broken;
- system integrity information required for safe recovery is unavailable.

---

# 4. Complete architecture

## 4.1 Layer model

```text
Firmware / BIOS / UEFI
        ↓
Bootloader
        ↓
Early Kernel
        ↓
CPU / Memory / Interrupt Infrastructure
        ↓
Kernel Core
        ↓
Device / Storage / Network Subsystems
        ↓
Userspace Init / Service Manager
        ↓
Display / Input Services
        ↓
Compositor / Window System
        ↓
Desktop Shell
        ↓
Settings / File Manager / Terminal / Store / Applications
```

## 4.2 Dependency rule

Preferred dependency direction:

`boot → kernel primitives → kernel services → device abstractions → userspace services → GUI/applications`

Examples:

- PMM must not know what a window is.
- Network drivers must not know what Settings looks like.
- Package management must work without the GUI.
- GUI must not directly access hardware.
- Error reporting from early boot must not require the GUI.

## 4.3 Failure containment hierarchy

Preferred containment:

`application → application restart → service restart → userspace subsystem recovery → kernel subsystem recovery → recovery boot → system stop`

The OS should stop at the lowest level that guarantees safety.

---

# 5. Repository organization

Target ownership:

```text
boot/                 boot entry and early CPU state
kernel/               privileged kernel implementation
userspace/            userspace services and applications
docs/                 subsystem specifications and test plans
site/                 public website
tests/                host/QEMU/integration tests when applicable
.github/              CI, release and project automation
Makefile              reproducible build orchestration
linker.ld             final kernel memory layout
README.md             short public project introduction
SB_OS_DESIGN.md       canonical master specification
```

Target kernel structure:

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
  sched/
  process/
  syscall/
  ipc/
  security/
  drivers/
    pci/
    usb/
    input/
    display/
    storage/
    network/
    audio/
  fs/
  net/
  time/
  power/
  log/
  panic/
```

Target userspace structure:

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
    logging/
    account/
    notification/
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

Directories are targets, not requirements to create empty placeholders.

---

# 6. Full construction order

The project must be built in dependency order.

## Phase 0 — Build foundation

- Toolchain.
- Freestanding compilation.
- Assembler.
- Linker script.
- Makefile.
- ISO creation.
- QEMU invocation.
- Serial logging.
- CI.
- Artifact validation.

## Phase 1 — Boot

- Boot protocol entry.
- CPU mode.
- Stack.
- Boot information preservation.
- Early output.
- Kernel layout.
- Reserved memory.

## Phase 2 — Memory

- PMM.
- VMM.
- Kernel heap.
- Memory diagnostics.

## Phase 3 — CPU/runtime

- GDT.
- TSS.
- IDT.
- Exceptions.
- PIC/APIC.
- Timer.
- Synchronization.
- SMP groundwork.

## Phase 4 — Execution

- Scheduler.
- Threads.
- Processes.
- Address spaces.
- Syscalls.
- ELF loader.
- Userspace init.
- Service manager.

## Phase 5 — Storage

- Block layer.
- Partition layer.
- Filesystem.
- VFS.
- File permissions.
- Persistent configuration.
- Installer/storage preparation.

## Phase 6 — Hardware I/O

- PCI.
- ACPI.
- Keyboard.
- Mouse.
- USB foundations.
- Display/framebuffer.
- Storage devices.
- Network devices.
- Audio.
- Power management.

## Phase 7 — GUI

- Text rendering.
- Fonts.
- Surfaces.
- Event system.
- Compositor.
- Window manager.
- Toolkit.
- Desktop shell.
- Notifications.
- Clipboard.
- File manager.

## Phase 8 — First boot/localization

- Configuration service.
- Locale service.
- Language catalogs.
- First-run popup.
- Persistence.
- Settings integration.
- Keyboard layout and input method groundwork.

## Phase 9 — Network

- NIC abstraction.
- Ethernet.
- IPv4.
- IPv6.
- ARP/ND.
- Routing.
- UDP.
- TCP.
- DNS.
- DHCP.
- Sockets.
- Network manager.
- Firewall/policy.

## Phase 10 — Software distribution

- Package format.
- Manifest.
- Repository metadata.
- Downloader.
- Verification.
- Transaction engine.
- Install/remove/update.
- SB Store.
- Cache management.

## Phase 11 — Security/recovery

- Privilege model.
- Permissions.
- Sandboxing boundaries.
- Package signing.
- Update signing.
- Crash diagnostics.
- Recovery mode.
- Rollback.
- Backup/export mechanisms.

## Phase 12 — Compatibility/performance

- Hardware matrix.
- GPU support.
- Power management.
- Boot performance.
- Memory footprint.
- Driver stability.
- Storage efficiency.
- Network efficiency.

## Phase 13 — Release

- Release candidate.
- Full CI.
- Clean install.
- Upgrade test.
- Recovery test.
- Hardware test.
- Documentation.
- Checksums/signatures.
- Release artifacts.
- Website publication.

---

# 7. Toolchain and build system

## 7.1 Freestanding kernel requirements

Kernel code must not accidentally depend on host libc/startup files/runtime.

The build must explicitly define:

- target architecture;
- ABI;
- freestanding mode;
- compiler/assembler versions or reproducible toolchain source;
- warning policy;
- optimization policy;
- relocation model;
- stack-protection policy if used;
- required compiler runtime functions.

If compiler-generated runtime helpers are required, they must be deliberately provided or linked in a controlled manner.

## 7.2 Assembly architecture correctness

The project has already encountered the class of failure where 32-bit assembly is asked to represent a 64-bit relocation. Prevent recurrence by making architecture assumptions explicit.

Rules:

- 32-bit boot code must not contain unrepresentable 64-bit relocations.
- 64-bit code must use the intended assembler mode.
- Linker symbols must be referenced using the correct operand width.
- Every boot relocation must be tested in CI.

## 7.3 Linker layout

Document and verify:

- entry point;
- `.text`;
- `.rodata`;
- `.data`;
- `.bss`;
- alignment;
- kernel end;
- boot stack;
- page tables;
- early allocator metadata;
- symbols exported to assembly.

After layout changes, CI should inspect the actual addresses.

## 7.4 Build targets

Equivalent functionality must exist for:

- normal build;
- clean build;
- ISO generation;
- QEMU boot;
- tests;
- artifact validation;
- debug build;
- release build.

---

# 8. Bootloader and firmware boundary

## 8.1 Supported boot paths

Initial priority:

1. QEMU test environment.
2. Generic x86_64 legacy/BIOS-compatible boot path if supported.
3. UEFI boot path.
4. Real hardware validation.

Do not claim UEFI/BIOS support until each path is actually tested.

## 8.2 Firmware responsibilities

The firmware owns platform initialization before the bootloader. SB must not assume that every platform exposes identical firmware behavior.

## 8.3 Boot responsibilities

The boot path must:

- enter the expected CPU state;
- establish a known stack;
- preserve required boot information;
- establish required initial paging;
- transfer control through a documented ABI;
- avoid initializing high-level services.

## 8.4 Boot memory reservation

Never give the PMM memory occupied by:

- boot code/data;
- stack;
- page tables;
- kernel image;
- Multiboot structures still in use;
- framebuffer;
- firmware-reserved memory;
- PMM metadata;
- crash/recovery buffers.

---

# 9. Physical Memory Manager (PMM)

## 9.1 Purpose

PMM owns physical page allocation.

## 9.2 Required operations

Conceptual API:

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

## 9.3 Initialization

Bootstrap with a known-safe region if necessary. Later integrate the complete physical memory map.

**Never reinitialize the allocator by clearing state that is already in use.**

## 9.4 Reservation order

1. Firmware-reserved regions.
2. Boot structures.
3. Kernel image.
4. Stack.
5. Page tables.
6. PMM metadata/bitmap.
7. Framebuffer.
8. Other explicitly reserved structures.
9. Remaining genuinely usable memory becomes free.

## 9.5 Invariants

- Every returned page is aligned.
- Reserved pages cannot be allocated.
- Freed pages become available exactly once.
- Invalid pages are rejected.
- Metadata cannot overlap free pages.
- Counters are consistent.
- Integer overflow is impossible or checked.
- Initialization cannot run forever.

## 9.6 PMM debugging procedure

If boot stops at PMM:

1. Confirm the last emitted checkpoint.
2. Confirm PMM static addresses.
3. Inspect linker map.
4. Check overlap with stack/page tables/kernel.
5. Check page count calculations.
6. Check bitmap size.
7. Check loop bounds.
8. Check integer overflow.
9. Check whether diagnostics are blocking.
10. Check whether PMM is being initialized twice.
11. Test a tiny fixed allocator.
12. Test with low QEMU RAM.
13. Test with larger QEMU RAM.
14. Re-run with memory layout assertions.

## 9.7 Tests

- first page;
- many pages;
- free/reallocate;
- exhaustion;
- invalid page;
- double free;
- reserved region;
- low-memory environment;
- large-memory environment;
- randomized allocation/free sequence.

---

# 10. Virtual Memory Manager (VMM)

## 10.1 Responsibilities

- page-table creation;
- mapping;
- unmapping;
- permissions;
- address-space lifecycle;
- TLB management;
- page fault handling;
- ownership of page-table pages through PMM.

## 10.2 Protection model

At minimum distinguish:

- kernel-only;
- user-readable;
- user-writable;
- executable where supported;
- non-executable where supported.

User processes must not arbitrarily access kernel memory.

## 10.3 Fault behavior

Invalid user access should generate a controlled page fault and eventually an application-level failure when safe.

Invalid kernel access should produce a kernel diagnostic/panic path unless a controlled recovery mechanism exists.

## 10.4 Tests

- map/unmap;
- permissions;
- page fault;
- address-space isolation;
- page-table cleanup;
- TLB invalidation;
- allocation failure;
- mapping alignment;
- double-unmap handling.

---

# 11. Kernel heap

Required properties:

- documented alignment;
- overflow checks;
- defined zero-size behavior;
- defined allocation failure;
- no hidden recursion through logging;
- fragmentation awareness;
- repeated alloc/free tests;
- interaction tests with PMM/VMM.

Provide page allocation separately from heap allocation. The heap must not become the only memory primitive.

---

# 12. CPU architecture layer

## 12.1 GDT

Define and test required code/data segments for kernel and userspace.

## 12.2 TSS

Provide per-CPU/kernel-stack data needed for safe privilege transitions.

## 12.3 IDT

Install handlers for CPU exceptions and hardware/software interrupts.

## 12.4 Exceptions

Each exception should capture enough state for diagnosis:

- vector;
- error code when present;
- instruction pointer;
- code segment;
- flags;
- stack pointer;
- relevant registers;
- current thread/process;
- subsystem state when safely available.

## 12.5 Interrupt controllers

Support the selected interrupt architecture through an abstraction so the rest of the kernel does not depend directly on one controller implementation.

## 12.6 Timer

Provide a monotonic timer facility for:

- scheduling;
- sleep;
- timeouts;
- performance measurement.

Wall-clock time must be a separate concept from monotonic elapsed time.

---

# 13. ACPI, SMP and platform discovery

Eventually support:

- ACPI table discovery;
- CPU enumeration;
- interrupt routing information;
- power state information;
- thermal information where safely available;
- multiple CPUs/cores.

SMP rollout:

1. Single CPU correctness.
2. Per-CPU data structures.
3. Secondary CPU startup.
4. Scheduler awareness.
5. Interrupt routing.
6. Synchronization stress testing.

Do not enable complex SMP behavior before synchronization primitives are reliable.

---

# 14. Synchronization and concurrency

Required primitives eventually include:

- spinlocks;
- mutexes;
- semaphores where useful;
- wait queues;
- atomic operations;
- reference counting;
- interrupt-safe locking rules.

Every lock must have documented ownership expectations.

Avoid lock-order inversions.

Never sleep while holding a lock that cannot legally be held across a sleep.

Add concurrency tests before enabling SMP-dependent services.

---

# 15. Scheduler, threads and processes

## 15.1 Thread

A thread owns execution context and scheduling state.

## 15.2 Process

A process owns an address space and process-level resources.

## 15.3 Scheduler

Minimum requirements:

- runnable queue;
- blocked state;
- sleep/timer state;
- context switch;
- fairness policy;
- idle thread;
- CPU utilization accounting.

Future priorities may include:

- interactive responsiveness;
- background tasks;
- power-aware scheduling.

## 15.4 Process lifecycle

Required states conceptually:

`created → runnable → running → blocked/sleeping → runnable → exited → reaped`

Resources must be cleaned at exit.

---

# 16. Syscall ABI

The userspace boundary must be explicit and versioned.

Required syscall families:

- process/thread control;
- memory mapping;
- file I/O;
- time;
- IPC;
- networking;
- device abstractions;
- configuration/service communication.

Every syscall must validate:

- pointers;
- lengths;
- handles;
- permissions;
- integer overflow;
- user/kernel address boundaries.

Define stable error codes and document whether errors are retryable.

Changing a public ABI requires a compatibility policy.

---

# 17. IPC and service architecture

SB should use explicit service boundaries rather than making every component a privileged monolith.

Potential IPC mechanisms:

- message passing;
- pipes;
- shared memory with synchronization;
- sockets/local endpoints;
- event notification.

The final mechanism may vary, but services must have:

- lifecycle;
- ownership;
- permissions;
- timeout behavior;
- restart policy;
- health state;
- diagnostic identity.

---

# 18. ELF/userspace loader

The executable loader must eventually:

- validate file format;
- validate architecture;
- validate program headers;
- create mappings;
- apply permissions;
- initialize stack;
- establish entry point;
- provide process startup information;
- reject malformed executables safely.

Malformed binaries must never cause arbitrary kernel memory access.

---

# 19. Userspace init and service manager

Startup should be dependency-aware and lazy.

Required service concepts:

- service identity;
- dependencies;
- startup condition;
- restart policy;
- failure state;
- logs;
- resource limits;
- shutdown ordering.

Only services necessary for a usable desktop should automatically start.

Installed optional applications must not automatically become permanent boot-time processes.

---

# 20. Time and clock system

Separate:

- monotonic time;
- wall-clock time;
- timezone;
- calendar/localization.

Required future capabilities:

- RTC initialization;
- time synchronization;
- timezone database;
- user-visible date/time formatting;
- timers;
- alarms where appropriate.

A broken wall clock must not break scheduler timing.

---

# 21. Storage architecture

Layers:

```text
Physical device
 ↓
Block layer
 ↓
Partition layer
 ↓
Filesystem driver
 ↓
VFS
 ↓
File APIs
 ↓
Applications
```

## 21.1 Required file operations

- open;
- close;
- read;
- write;
- seek;
- stat;
- create;
- rename;
- delete;
- directory enumeration;
- permissions;
- timestamps;
- durable write.

## 21.2 Atomic configuration

Configuration updates should use:

`write temporary → validate → flush as required → atomically replace → recover stale temporary file`

where supported.

## 21.3 Filesystem recovery

Interrupted writes must not intentionally produce an unrecoverable configuration state.

Filesystem checks/recovery tools must be available outside the normal desktop when necessary.

---

# 22. Installer and disk management

The project must eventually provide a safe installation path.

Installer responsibilities:

- detect disks;
- clearly identify target disk;
- distinguish removable/secondary disks;
- show destructive operations clearly;
- partition safely;
- create required filesystem structures;
- install boot files;
- copy base system;
- create initial configuration;
- verify installation;
- reboot into installed system.

Never make destructive disk operations implicit.

The installer should support a safe test path for QEMU virtual disks.

Future options may include:

- erase and install;
- manual partitioning;
- existing-system coexistence where technically supported;
- recovery/repair mode.

---

# 23. Input architecture

Abstract hardware input from GUI input.

Initial devices:

- keyboard;
- mouse.

Later:

- touch;
- gamepads/controllers;
- tablets;
- hot-plug devices;
- Bluetooth input.

Input events should contain:

- device identity;
- timestamp;
- event type;
- key/button/code;
- state;
- pointer coordinates where applicable.

---

# 24. Internationalization and input methods

## 24.1 Initial languages

- Japanese
- English
- Chinese
- Spanish

## 24.2 Localization requirements

Use stable message IDs such as:

```text
ui.welcome.title
settings.language
error.network.timeout
panic.kernel.page_fault
```

Support:

- fallback language;
- plural rules;
- date/time formatting;
- number formatting;
- text direction metadata;
- Unicode;
- font fallback;
- string length expansion;
- locale-specific keyboard settings.

## 24.3 Japanese/Chinese input

Language selection and text rendering are not sufficient for full usability.

The architecture must eventually allow input methods/IME-style composition so users can type languages that require composition.

The base OS should not need every large language resource installed if downloadable language packs are practical.

---

# 25. Display architecture

Target layers:

`GPU/framebuffer driver → display service → compositor → window system → toolkit`

## 25.1 Fallback

A generic framebuffer path should allow basic display where a vendor-specific accelerated driver is unavailable and the platform exposes a usable framebuffer.

## 25.2 Resolution/hotplug

Eventually support:

- resolution changes;
- refresh rate selection;
- multiple displays;
- display orientation;
- hotplug events;
- scaling.

---

# 26. Graphics acceleration

Separate graphics API from vendor driver.

Potential long-term support:

- Intel graphics;
- AMD graphics;
- NVIDIA consumer GPUs;
- NVIDIA professional GPUs;
- modern high-end GPUs;
- virtual GPU/QEMU devices.

The project should not promise a specific RTX generation merely because the OS boots on generic x86_64.

Driver compatibility must be proven per driver family and tested on real or virtual hardware.

A missing acceleration driver should degrade gracefully when a generic path exists.

---

# 27. GUI architecture

Layers:

`Display Driver → Display Service → Compositor → Window System → Toolkit → Desktop Shell → Apps`

Minimum primitives:

- surfaces;
- windows;
- text;
- fonts;
- labels;
- buttons;
- text fields;
- lists;
- select/drop-down controls;
- dialogs;
- menus;
- notifications;
- scroll containers;
- keyboard focus;
- pointer input;
- accessibility metadata.

GUI applications must not directly manipulate hardware.

## 27.1 Event model

GUI should be event-driven.

Avoid:

- busy waiting;
- fixed polling loops for input;
- blocking the compositor on network operations;
- blocking the entire desktop while an application performs disk I/O.

## 27.2 Window manager

Must manage:

- creation;
- focus;
- stacking;
- minimize/maximize/close;
- move/resize;
- fullscreen;
- workspace concepts if implemented;
- crash cleanup.

---

# 28. Desktop shell

Minimum desktop functions:

- application launcher;
- task/window management;
- system status area;
- notification system;
- settings access;
- file manager access;
- terminal access;
- power controls;
- network status;
- clock;
- basic user/session controls.

The shell must remain usable if an optional application crashes.

---

# 29. Accessibility

Accessibility is part of architecture, not a later visual polish item.

Plan for:

- keyboard navigation;
- visible focus;
- scalable text;
- display scaling;
- high-contrast themes;
- reduced-motion preference;
- screen-reader hooks;
- semantic UI labels;
- color-independent status indicators.

The exact implementation may arrive later, but GUI APIs should not make it impossible.

---

# 30. Clipboard and desktop integration

Provide a controlled clipboard service rather than allowing applications to access arbitrary application memory.

Potential clipboard types:

- plain text;
- rich text;
- images;
- files/URI lists.

Clipboard lifetime and privacy behavior must be defined.

---

# 31. Notifications

Notification service responsibilities:

- application identity;
- severity;
- title/body;
- timestamp;
- action buttons;
- persistence policy;
- quiet mode;
- accessibility output.

Kernel-level failures must not rely on the normal notification service if that service may itself be broken.

---

# 32. Settings architecture

Settings categories:

- System
- Appearance
- Display
- Sound
- Network
- Keyboard & Mouse
- Input Methods
- Storage
- Applications
- Privacy & Security
- Accounts
- Updates
- Language & Region
- Performance
- Developer
- Recovery

Every setting needs:

- stable identifier;
- type/schema;
- default;
- validation;
- persistence policy;
- owning subsystem;
- UI description;
- migration behavior if its format changes.

No duplicate source of truth.

---

# 33. User/account/permission model

The OS eventually needs a real identity model.

Requirements:

- user identity;
- groups/roles;
- authentication mechanism;
- session ownership;
- file permissions;
- privileged operations;
- service identity.

Normal applications should run without unrestricted privileged access.

The GUI must never imply that a cosmetic role is a security boundary.

Authentication secrets must be stored through a dedicated protected mechanism, not ordinary configuration files.

---

# 34. Terminal and shell

SB Desktop includes a real terminal.

Required direction:

- process execution;
- environment variables;
- pipes;
- redirection;
- filesystem commands;
- process tools;
- network tools;
- package commands;
- diagnostics;
- recovery commands;
- scripting eventually.

GUI and terminal should call the same underlying services wherever practical.

The terminal is **not** the first-run configuration mechanism.

---

# 35. Networking

Target stack:

`NIC → link layer → IPv4/IPv6 → ARP/ND → routing → UDP/TCP → DNS/DHCP → sockets → network manager`

Requirements:

- Ethernet;
- IPv4;
- IPv6;
- DHCP;
- static addresses;
- DNS;
- routing;
- loopback;
- sockets;
- firewall/policy;
- diagnostics.

Later:

- Wi-Fi;
- Bluetooth networking;
- VPN interfaces;
- advanced routing.

Every network operation must have timeout/error semantics and must not freeze the desktop.

---

# 36. Network manager GUI/CLI parity

The same underlying configuration model must power:

- GUI network settings;
- terminal commands;
- automatic configuration;
- diagnostics.

Do not create a GUI-only network configuration database that the terminal cannot understand.

---

# 37. Audio architecture

Audio is a separate subsystem.

Target layers:

`audio hardware driver → kernel/device service → userspace audio service → mixer/session policy → applications`

Required concepts:

- output devices;
- input devices;
- volume;
- mute;
- per-application routing eventually;
- device hotplug;
- sample format/rate handling.

Audio failure must not crash the desktop.

---

# 38. USB and hotplug

The architecture should support device discovery/removal without requiring reboot where technically possible.

Requirements:

- device identity;
- enumeration;
- driver matching;
- attach/detach lifecycle;
- safe resource cleanup.

Hot-unplug must not leave dangling kernel pointers.

---

# 39. Power management

Eventually support:

- shutdown;
- reboot;
- suspend;
- hibernate if technically feasible;
- CPU power states;
- display power;
- battery reporting;
- thermal information;
- laptop lid events.

Shutdown/reboot must use dependency-aware service termination.

Never cut power while persistent data is known to be mid-transaction unless forced recovery requires it.

---

# 40. Package manager

Architecture:

`SB Core → package manager → repository metadata → downloader → verification → dependency solver → transaction → filesystem → package database`

Package metadata must include at least:

- package ID;
- name;
- version;
- architecture;
- dependencies;
- conflicts;
- files;
- optional dependencies;
- integrity hash;
- signature/provenance information.

Required operations:

- search;
- install;
- remove;
- upgrade;
- list;
- inspect;
- verify;
- rollback where feasible;
- clean cache;
- repair database.

Transactions should be interruptible/recoverable.

---

# 41. SB Store

SB Store is a GUI front end to the package/repository system, not a separate package universe.

Required concepts:

- search;
- categories;
- installed state;
- version;
- size;
- dependencies;
- screenshots/description later;
- install;
- remove;
- update;
- cancellation;
- progress;
- error details.

The base OS should remain usable without an internet connection. Previously installed software must not disappear merely because the repository is unavailable.

---

# 42. Fast downloads

Optimize downloads through:

- compact metadata;
- compression;
- caching;
- resumable downloads;
- range requests where supported;
- mirrors/CDN later;
- parallel transfers when beneficial;
- deduplication where practical.

Never sacrifice cryptographic/integrity verification for speed.

---

# 43. Repository and package security

Official packages/metadata should eventually use cryptographic signing.

Threats to consider:

- malicious mirror;
- corrupted download;
- compromised package;
- rollback to vulnerable version;
- dependency confusion;
- metadata tampering.

The client must verify identity/integrity before installation.

---

# 44. Driver framework

Drivers must sit behind stable interfaces.

Initial priority:

- PCI;
- framebuffer/display;
- keyboard;
- mouse;
- storage;
- Ethernet;
- USB foundations.

Later:

- Wi-Fi;
- audio;
- Intel graphics;
- AMD graphics;
- NVIDIA graphics;
- modern storage controllers;
- additional PCIe/USB devices.

Driver failure should be isolated when possible.

A driver must declare:

- hardware IDs;
- capabilities;
- required resources;
- lifecycle;
- failure state;
- supported versions.

---

# 45. PCI and hardware discovery

PCI enumeration must provide a hardware inventory abstraction for the rest of the system.

Record:

- bus/device/function;
- vendor/device IDs;
- class/subclass;
- BAR information where applicable;
- interrupt information;
- driver binding state.

Do not assume a device exists merely because a common PC usually has one.

---

# 46. Security architecture

Security boundaries:

`firmware/boot → kernel → privileged services → ordinary userspace → applications`

Required directions:

- memory isolation;
- syscall validation;
- file permissions;
- service privileges;
- package verification;
- secure updates;
- firewall policy;
- least privilege;
- protected secrets;
- sanitized logs.

## 46.1 Threat model

Document threats against:

- malicious local application;
- compromised package;
- malicious network server;
- corrupted storage;
- accidental user action;
- driver bugs;
- kernel bugs;
- physical loss of the device.

Security decisions should be made according to threat severity rather than aesthetic complexity.

---

# 47. Logging architecture

Structured logs should contain:

- timestamp;
- severity;
- component;
- event/error ID;
- boot ID;
- session ID where appropriate;
- process/thread where available;
- message;
- relevant sanitized context.

Severity classes should be stable enough for filtering.

Do not log:

- passwords;
- authentication tokens;
- private keys;
- session cookies;
- unnecessary personal data.

---

# 48. Crash dumps and diagnostics

Serious failures should produce a machine-readable diagnostic record when possible.

Include:

- OS build;
- kernel build;
- hardware inventory;
- CPU information;
- memory information;
- driver versions;
- recent subsystem events;
- exception state;
- process/thread;
- error ID;
- boot/session ID.

Allow users to save/export a support report.

The report should have a privacy review/filtering step.

---

# 49. Recovery architecture

Recovery priorities:

1. Restart failed application.
2. Restart failed userspace service.
3. Restart network/display service when safe.
4. Roll back failed package transaction.
5. Boot recovery environment.
6. Run filesystem checks.
7. Restore known-good configuration.
8. Reinstall damaged system components.
9. Kernel panic only if no safe continuation exists.

Every recovery mechanism needs a termination condition. Never create an infinite “repair → reboot → repair” loop.

---

# 50. Configuration backup and migration

Configuration formats must be versioned.

When a configuration schema changes:

1. Detect old version.
2. Validate it.
3. Migrate it.
4. Keep a recovery copy when practical.
5. Validate new format.
6. Atomically activate it.

Never assume an old configuration is valid merely because it is syntactically readable.

---

# 51. Update system

Target transaction:

`metadata → verification → dependency resolution → download → staging → validation → activation → health check → rollback if necessary`

Requirements:

- signed metadata;
- integrity hashes;
- dependency checks;
- atomic activation where practical;
- interrupted-update recovery;
- rollback;
- progress reporting;
- clear error messages.

A failed update must not intentionally leave the machine in an unknown half-updated state.

---

# 52. Backup and restore

The OS must distinguish:

- system files;
- user files;
- configuration;
- application data;
- package state;
- recovery metadata.

Future backup tooling should let users choose what to preserve.

Restore must validate compatibility before overwriting active system state.

---

# 53. Data minimization and cleanup

Classify data by ownership:

- user data;
- system data;
- package state;
- cache;
- temporary data;
- logs;
- recovery data.

Safe cleanup candidates:

- expired temporary files;
- obsolete package caches;
- orphaned package artifacts;
- stale generated data.

Never automatically delete unknown files or user documents merely because they are large or old.

Cleanup must have a dry-run/report mode where feasible.

---

# 54. Privacy

Privacy defaults should favor collecting less data.

The system should clearly distinguish:

- local diagnostics;
- optional support upload;
- update communication;
- telemetry if ever introduced.

No hidden telemetry should be required for the core OS.

Any future telemetry feature must be explicitly documented and controllable.

---

# 55. Application isolation

The long-term application model should support different trust levels.

Possible classes:

- trusted system service;
- normal application;
- sandboxed application;
- developer/debug application.

The architecture should allow sandboxing without requiring every GUI application to run as a privileged process.

---

# 56. Developer mode

Advanced users/developers may need:

- verbose boot logs;
- serial logging;
- debug symbols;
- diagnostic tools;
- driver debugging;
- kernel tracing;
- performance counters;
- development packages.

Developer mode must not silently weaken ordinary user security.

---

# 57. Performance engineering

Measure before optimizing.

Track:

- boot stages;
- scheduler latency;
- page fault cost;
- allocation latency;
- filesystem latency;
- network latency;
- compositor frame time;
- application startup;
- idle wakeups;
- memory fragmentation.

Optimize in this order:

1. Correctness.
2. Major architectural bottlenecks.
3. User-visible latency.
4. Memory footprint.
5. CPU efficiency.
6. Minor size/byte-level optimizations.

---

# 58. Compatibility strategy

Initial stable target:

**generic x86_64 PC**

Do not immediately create dozens of ISO variants.

Use capability detection and graceful fallback.

Future compatibility classes may include:

- ordinary laptops;
- ordinary desktops;
- gaming PCs;
- workstations;
- professional GPU systems;
- server systems.

A release variant should exist only when real hardware differences justify it.

---

# 59. GPU strategy

The long-term target includes broad GPU support, including high-performance NVIDIA/AMD/Intel hardware where technically and legally feasible.

Potential NVIDIA classes include consumer and professional generations, including RTX-class hardware.

However:

- generic x86_64 boot does not equal GPU support;
- framebuffer output does not equal acceleration;
- PCI detection does not equal a usable graphics driver;
- a driver must be implemented, integrated, tested and documented before support is claimed.

GPU acceleration must have a fallback path where possible.

---

# 60. Installer/live environment/recovery environment

The final distribution should distinguish:

- installer/live environment;
- installed desktop system;
- recovery environment.

The installer should be able to operate without relying on the installed system being healthy.

The recovery environment should be able to:

- inspect disks;
- inspect logs;
- repair configuration;
- repair/reinstall boot files;
- check filesystems;
- restore packages;
- roll back updates;
- export diagnostics.

---

# 61. Release artifact strategy

Potential artifacts:

- bootable ISO;
- checksum files;
- signature files;
- QEMU test image where useful;
- source archive;
- debug symbols for developers;
- release notes;
- hardware compatibility notes.

Future hardware-specific releases should be generated from the same source and clearly labeled.

---

# 62. Reproducible build and supply chain

The release process should eventually record:

- source commit;
- toolchain version;
- build environment;
- dependency versions;
- generated artifact hashes;
- build timestamp policy;
- package manifest;
- signing identity.

Aim for reproducible or at least independently verifiable builds.

Do not depend on an undocumented developer machine.

---

# 63. SB website and official ecosystem

The project should eventually publish through free/open infrastructure where practical.

Official project resources should have a single source-of-truth relationship with the repository.

Website should provide:

- project explanation;
- download page;
- installation guide;
- hardware compatibility;
- documentation;
- changelog;
- security advisories;
- contribution guide;
- support/community links.

GitHub Pages can serve as the initial official website when cost is a constraint.

Community services may include Discord and social media, but core technical truth must remain in the repository/documentation rather than depending on a chat server.

---

# 64. Documentation architecture

The repository should maintain both:

1. This master document.
2. Smaller subsystem documents.

Subsystem documents may cover:

- boot;
- memory;
- scheduler;
- ELF;
- filesystem;
- drivers;
- GUI;
- first boot;
- settings;
- package manager;
- store;
- errors;
- release;
- ecosystem.

If a subsystem document conflicts with this master specification, the conflict must be resolved explicitly; do not let contradictory specifications silently accumulate.

---

# 65. Test architecture

Testing must exist at multiple levels.

## 65.1 Unit tests

Use where the code can be isolated safely.

Examples:

- bitmap operations;
- parsers;
- configuration validation;
- package manifest parsing;
- dependency solver;
- localization lookup.

## 65.2 Kernel integration tests

Examples:

- PMM + VMM;
- interrupts + scheduler;
- filesystem + storage;
- syscall + userspace.

## 65.3 QEMU tests

Examples:

- boot;
- memory;
- exceptions;
- userspace launch;
- storage;
- networking;
- framebuffer;
- GUI startup.

## 65.4 Real hardware tests

Eventually test representative:

- laptop;
- desktop;
- gaming PC;
- different CPU generations;
- different GPUs;
- different storage devices;
- different network hardware.

A QEMU pass is not automatically a real-hardware pass.

---

# 66. Mandatory end-to-end test list

Before first major stable release:

1. Clean ISO build.
2. ISO integrity.
3. QEMU cold boot.
4. QEMU reboot.
5. QEMU shutdown.
6. Clean disk installation.
7. First graphical startup.
8. Language selector.
9. Japanese selection.
10. English selection.
11. Chinese selection.
12. Spanish selection.
13. Language persistence after reboot.
14. Keyboard/mouse.
15. Display.
16. Settings persistence.
17. Filesystem operations.
18. Network configuration.
19. DNS.
20. Terminal execution.
21. Package installation.
22. Package removal.
23. Package update.
24. Application crash recovery.
25. Service restart.
26. Diagnostic report.
27. Recovery mode.
28. Controlled kernel exception.
29. Failed update rollback.
30. Clean shutdown during active writes.
31. Interrupted package transaction.
32. Invalid configuration recovery.
33. Low-memory boot.
34. High-memory boot.
35. Multiple CPU test when SMP enabled.
36. Hardware hotplug tests where supported.
37. Security/permission tests.
38. Artifact checksum/signature verification.

---

# 67. Failure investigation procedure

When something fails, do not randomly rewrite code.

Use:

```text
1. Reproduce
2. Record exact environment
3. Identify last successful checkpoint
4. Reduce to smallest failing subsystem
5. Inspect logs/registers/state
6. Form a falsifiable hypothesis
7. Add minimal diagnostic instrumentation
8. Reproduce
9. Confirm root cause
10. Fix root cause
11. Remove unnecessary diagnostics
12. Add regression test
13. Re-run broader test suite
14. Document the lesson if architecture changed
```

## 67.1 Timeouts

A timeout is not evidence that “the system is slow.”

First determine whether the system:

- entered an infinite loop;
- faulted silently;
- deadlocked;
- blocked on I/O;
- waited for a nonexistent device;
- corrupted control flow;
- exhausted memory;
- stopped emitting logs because the logger failed.

---

# 68. Current PMM incident recovery rule

The project has previously experienced a QEMU boot path that reached PMM initialization and then stopped until the CI timeout.

The correct response is:

- preserve the diagnostic evidence;
- inspect actual linker addresses;
- check bitmap/static-data placement;
- check stack/page-table overlap;
- check initialization order;
- check duplicate initialization;
- check loop bounds and integer overflow;
- check whether diagnostic output itself blocks;
- reproduce with a minimal allocator.

Do not “fix” the incident by deleting PMM or skipping memory management.

---

# 69. AI development protocol

Every AI working in the repository must follow this exact loop:

```text
READ MASTER SPEC
    ↓
INSPECT REPOSITORY
    ↓
CHECK CURRENT CI / TESTS
    ↓
IDENTIFY REAL CURRENT STATE
    ↓
IDENTIFY SMALLEST BLOCKER
    ↓
IMPLEMENT
    ↓
BUILD
    ↓
RUN TEST
    ↓
READ LOG
    ↓
FIX ROOT CAUSE
    ↓
ADD REGRESSION TEST
    ↓
UPDATE DOCS IF NEEDED
    ↓
REPORT ACTUAL STATE
```

AI must never:

- invent successful tests;
- claim a commit that was not created;
- claim code was changed when it was not;
- call an untested feature complete;
- remove a failing test just to obtain green CI;
- hide a crash;
- silently replace architecture;
- add a second source of truth for an existing subsystem;
- create fake compatibility claims;
- treat a screenshot as proof of functional implementation.

---

# 70. Change management

Every architectural change should answer:

1. What problem does it solve?
2. Why is the current architecture insufficient?
3. What dependencies change?
4. What APIs change?
5. What persistent data formats change?
6. What tests must change?
7. What compatibility risks exist?
8. What rollback path exists?

If a change is only a refactor, preserve behavior and prove it with tests.

---

# 71. Definition of implementation states

Use these states consistently:

### PLANNED

Architecture described but no meaningful implementation.

### SKELETON

Interfaces/files exist but feature is not functional.

### PARTIAL

Some real behavior exists but required scenarios remain missing.

### FUNCTIONAL

Feature works in its intended test environment.

### VERIFIED

Feature passes automated and relevant runtime tests.

### RELEASE-READY

Feature passes required hardware/security/recovery/documentation gates.

Never use “done” for PLANNED, SKELETON or PARTIAL.

---

# 72. Subsystem acceptance criteria

Every subsystem must define:

- purpose;
- inputs;
- outputs;
- ownership;
- dependencies;
- invariants;
- error conditions;
- recovery behavior;
- tests;
- performance expectations;
- security expectations;
- persistence/compatibility behavior if applicable.

A subsystem without these cannot be considered architecturally complete.

---

# 73. Release gates

A release candidate may proceed only when:

- build is reproducible enough for the project standard;
- ISO generation succeeds;
- QEMU boot succeeds;
- kernel smoke tests succeed;
- no known boot-critical regression exists;
- first-run setup works;
- persistence works;
- terminal works;
- storage works;
- networking works at the supported level;
- package installation works;
- recovery paths are tested;
- diagnostics work;
- artifact hashes/signatures are generated as required;
- documentation matches actual behavior.

Known limitations must be published instead of hidden.

---

# 74. Final user-level definition of done

SB Desktop v1 is complete only when a normal user can:

1. Boot a supported x86_64 computer or QEMU.
2. Reach a graphical desktop without terminal setup.
3. See the first-run language selector.
4. Select Japanese, English, Chinese or Spanish.
5. Continue into the desktop.
6. Reboot and retain the selected language.
7. Use keyboard and mouse.
8. Use the display system.
9. Configure networking.
10. Browse and manage files.
11. Use a real terminal.
12. Install/remove optional software.
13. Change detailed system settings.
14. Receive understandable normal error messages.
15. Obtain detailed technical diagnostics for serious failures.
16. Recover from supported application/service/package failures.
17. Safely update the system.
18. Avoid unnecessary base-system components consuming resources.
19. Keep user data safe through ordinary failures and supported recovery operations.
20. Understand what the OS is doing without needing to be a kernel developer.

Only after this foundation is genuinely stable should broad hardware expansion and specialized variants become the primary development direction.

---

# 75. Long-term expansion after SB Desktop v1

Once the first complete stable desktop exists, expansion may branch from the same foundation toward:

- gaming optimization;
- workstation optimization;
- professional GPU support;
- server-oriented configurations;
- high-memory systems;
- data-center hardware;
- specialized releases;
- additional architectures if the kernel abstraction permits it.

These are **future branches**, not excuses to interrupt the current SB Desktop completion path.

The rule is:

`Finish one complete stable system → stabilize it → measure it → branch it → add specialized capabilities.`

---

# 76. Master implementation checklist

Use this as the final progress checklist. A box may be checked only when the corresponding acceptance criteria are actually satisfied.

## Build

- [ ] Toolchain documented
- [ ] Freestanding build verified
- [ ] Assembly architecture verified
- [ ] Linker layout verified
- [ ] ISO build verified
- [ ] QEMU invocation verified
- [ ] CI build verified

## Boot

- [ ] Boot protocol verified
- [ ] CPU initialization verified
- [ ] Stack verified
- [ ] Boot information preserved
- [ ] Kernel layout verified
- [ ] Boot memory reservations verified
- [ ] BIOS path tested if supported
- [ ] UEFI path tested if supported

## Memory

- [ ] PMM
- [ ] PMM reservations
- [ ] PMM allocation/free
- [ ] PMM stress tests
- [ ] VMM
- [ ] Page protection
- [ ] Page faults
- [ ] Address-space isolation
- [ ] Kernel heap
- [ ] Memory diagnostics

## CPU/runtime

- [ ] GDT
- [ ] TSS
- [ ] IDT
- [ ] Exceptions
- [ ] Interrupt controller
- [ ] Timer
- [ ] Synchronization
- [ ] SMP groundwork

## Execution

- [ ] Scheduler
- [ ] Threads
- [ ] Processes
- [ ] Syscalls
- [ ] ABI documentation
- [ ] ELF loader
- [ ] Userspace init
- [ ] Service manager
- [ ] IPC

## Storage

- [ ] Block layer
- [ ] Partition layer
- [ ] Filesystem
- [ ] VFS
- [ ] Permissions
- [ ] Persistent configuration
- [ ] Atomic writes
- [ ] Installer
- [ ] Recovery storage tools

## Hardware

- [ ] PCI
- [ ] ACPI
- [ ] Keyboard
- [ ] Mouse
- [ ] USB
- [ ] Hotplug
- [ ] Display/framebuffer
- [ ] Audio
- [ ] Network device
- [ ] Power management

## GUI

- [ ] Text rendering
- [ ] Font system
- [ ] Unicode
- [ ] Input events
- [ ] Surfaces/windows
- [ ] Compositor
- [ ] Window manager
- [ ] Toolkit
- [ ] Desktop shell
- [ ] Notifications
- [ ] Clipboard
- [ ] File manager
- [ ] Accessibility foundations

## First boot

- [ ] Clean-install detection
- [ ] Graphical language popup
- [ ] Select/drop-down control
- [ ] Japanese
- [ ] English
- [ ] Chinese
- [ ] Spanish
- [ ] Atomic configuration save
- [ ] Persistence
- [ ] Corrupt-config recovery
- [ ] Settings integration

## Settings/localization

- [ ] Stable setting IDs
- [ ] Schema validation
- [ ] Migration
- [ ] Locale service
- [ ] Fallback language
- [ ] Pluralization
- [ ] Date/time formatting
- [ ] Input methods
- [ ] Keyboard layouts

## Network

- [ ] Ethernet
- [ ] IPv4
- [ ] IPv6
- [ ] ARP/ND
- [ ] Routing
- [ ] UDP
- [ ] TCP
- [ ] DNS
- [ ] DHCP
- [ ] Sockets
- [ ] Network manager
- [ ] GUI/CLI parity
- [ ] Firewall/policy

## Software

- [ ] Package format
- [ ] Manifest
- [ ] Repository metadata
- [ ] Downloader
- [ ] Integrity verification
- [ ] Signatures
- [ ] Dependency resolution
- [ ] Transactions
- [ ] Install
- [ ] Remove
- [ ] Update
- [ ] Rollback
- [ ] Cache management
- [ ] SB Store

## Security/recovery

- [ ] Privilege model
- [ ] Permissions
- [ ] Secret storage
- [ ] Package trust
- [ ] Update trust
- [ ] Firewall
- [ ] Logging
- [ ] Crash diagnostics
- [ ] Support report
- [ ] Application recovery
- [ ] Service recovery
- [ ] Recovery environment
- [ ] Configuration rollback
- [ ] Update rollback
- [ ] Backup/restore

## Performance

- [ ] Boot benchmark
- [ ] Idle RAM benchmark
- [ ] Idle CPU benchmark
- [ ] Background wakeup measurement
- [ ] GUI latency measurement
- [ ] Application startup benchmark
- [ ] Storage benchmark
- [ ] Network benchmark
- [ ] Package download benchmark
- [ ] Base-image size tracking

## Release

- [ ] Clean build
- [ ] Full CI
- [ ] Clean installation
- [ ] Upgrade
- [ ] Recovery
- [ ] Hardware compatibility test
- [ ] Artifact hashes
- [ ] Artifact signatures
- [ ] Release notes
- [ ] Known limitations
- [ ] Installation guide
- [ ] Recovery guide
- [ ] Website publication

---

# 77. Recovery procedure when project context is lost

If an AI/developer receives only this repository and no previous conversation:

1. Read this file completely.
2. Read `README.md`.
3. Inspect `.github/workflows/`.
4. Inspect `Makefile` and linker script.
5. Inspect `boot/`.
6. Inspect `kernel/`.
7. Inspect current tests.
8. Run the build.
9. Run QEMU smoke tests.
10. Record the first actual failure.
11. Map that failure to the roadmap above.
12. Fix the earliest unfinished prerequisite.
13. Do not jump ahead to GUI features if the kernel cannot safely boot.
14. Do not delete difficult subsystems merely to get a demo.
15. Continue until the release gates are satisfied.

The repository is the implementation. This file is the architectural memory.

---

# 78. Final project rule

SB is not finished when it can display a desktop screenshot.

SB is finished when the system is:

- bootable;
- memory-safe within its supported guarantees;
- usable;
- configurable;
- recoverable;
- diagnosable;
- updateable;
- modular;
- reasonably lightweight;
- secure enough for its stated release scope;
- documented;
- tested;
- reproducibly distributable.

The guiding development sequence is:

**Build one complete, trustworthy SB Desktop first. Then expand it.**

This document must evolve with the architecture. When the implementation proves that a design decision is wrong, update the specification and record the reason rather than allowing the code and documentation to silently diverge.
