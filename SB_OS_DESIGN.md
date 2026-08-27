# SuiraBox OS — Ultimate Master Specification

> **Document role:** This file is the single highest-level specification, recovery record, product definition, architecture reference, construction plan, UX definition, compatibility plan, and project memory for **SuiraBox OS (SB Desktop)**.
>
> **Primary purpose:** Preserve the complete intent of the project so that a new developer or AI can understand not only what files exist today, but what SB is supposed to become, why it is designed that way, how every major subsystem fits together, what the user should experience, what happens when something fails, how the system is extended, and how a release is proven ready.
>
> **Current active product:** SB Desktop.
>
> **Separate CUI-only operating system:** not an active development target. SB Desktop itself must contain a complete terminal and CLI because advanced users need command-line control.
>
> **Implementation-status rule:** This document defines intended behavior and acceptance criteria. It does not prove implementation. Real status is determined by source code, build output, tests, QEMU, CI and hardware verification.

---

# PART 0 — HOW TO READ THIS DOCUMENT

## 0.1 What this document is

This document is the authoritative description of the SB product and its intended implementation.

It is not merely a README, tutorial or checklist. It is intended to answer all of the following:

- What is SB?
- Who is SB for?
- What should the finished desktop look and behave like?
- What exists inside the OS?
- What code belongs in the kernel, userspace, drivers, GUI and package ecosystem?
- What data does the system store?
- What are the lifecycle rules for that data?
- How do subsystems communicate?
- How does boot proceed?
- How is memory owned and reclaimed?
- How are processes isolated?
- How are drivers attached and removed?
- How does the GUI start?
- How does first-run setup work?
- How are language and keyboard layout handled?
- How are errors displayed?
- When is BSOD used?
- When is RSOD used?
- How are packages downloaded, verified, installed and rolled back?
- How does SB remain lightweight?
- How does SB support many hardware classes?
- How is Minecraft optimization integrated without turning the whole OS into a single-purpose system?
- How does an AI continue the project without reinventing its architecture?

## 0.2 Status vocabulary

Every feature should use one of these states in project tracking:

`IDEA` → concept only
`SPECIFIED` → architecture/behavior defined
`SKELETON` → code structure exists
`PARTIAL` → part works
`FUNCTIONAL` → end-to-end behavior works in its supported environment
`RUNTIME-VERIFIED` → automated runtime verification passes
`HARDWARE-VERIFIED` → real hardware verification passes
`RELEASE-READY` → release acceptance criteria pass

Do not use “done” for anything below RELEASE-READY when describing a release-quality feature.

## 0.3 Source-of-truth hierarchy

When information conflicts:

1. The actual repository implementation determines what exists.
2. Tests and CI determine what has been verified.
3. This file determines intended product behavior and architecture.
4. Individual `docs/*.md`, issues and conversation history provide supplemental detail and history.

If code violates this specification, do not silently pretend that the specification was implemented. Either fix the code or explicitly change the specification through a documented architectural decision.

---

# PART I — PRODUCT AND USER EXPERIENCE

# 1. PRODUCT IDENTITY

## 1.1 Names

- Project: **SuiraBox OS**
- Short name: **SB**
- Current desktop product: **SB Desktop**
- Community/project identity: **Suiram**
- Current repository: `godrenkon/SuiraBox-OS`

## 1.2 Product objective

SB is an open-source desktop operating system intended to be:

- lightweight;
- fast;
- understandable;
- configurable;
- modular;
- user-oriented;
- recoverable;
- secure;
- broadly compatible;
- suitable for normal desktop use;
- suitable for gaming and Minecraft workloads;
- extensible without forcing unnecessary software into the base installation.

## 1.3 Core philosophy

The base OS should contain the minimum required foundation for a useful and reliable computer.

Additional capabilities should be installable on demand rather than permanently consuming base-system resources.

The design should resemble a toolkit more than an inflexible monolith:

```text
Small stable base
    ↓
Detect capabilities
    ↓
Choose needed components
    ↓
Download only what is required
    ↓
Verify
    ↓
Install transactionally
    ↓
Automatically integrate
    ↓
Use
```

This modularity must not become fragmentation for its own sake.

A component becomes separately installable only when doing so materially improves one or more of:

- base size;
- memory usage;
- startup time;
- security isolation;
- updateability;
- reliability;
- maintainability;
- user choice.

## 1.4 User-choice principle

SB should not force one workflow unnecessarily.

Advanced users can use the terminal and detailed settings.

Ordinary users should be able to complete ordinary tasks entirely through GUI controls.

The existence of a powerful CLI must never make the GUI intentionally incomplete.

## 1.5 “RPG toolkit” principle

The user should conceptually be able to build a system that fits their needs:

```text
Core
 ├─ required desktop components
 ├─ selected language
 ├─ selected drivers
 ├─ selected applications
 ├─ selected development tools
 ├─ selected gaming components
 └─ selected performance features
```

The OS must remain coherent. Optional modules use defined interfaces and cannot arbitrarily overwrite another subsystem's ownership.

---

# 2. FINISHED SB DESKTOP — WHAT THE USER SHOULD GET

## 2.1 High-level boot experience

Target user-visible sequence:

```text
Power On
 ↓
Firmware
 ↓
Bootloader
 ↓
Kernel
 ↓
Hardware discovery
 ↓
Memory / CPU / Interrupt infrastructure
 ↓
Kernel services
 ↓
Userspace init
 ↓
Storage / configuration
 ↓
Display / input
 ↓
Compositor
 ↓
Desktop shell
 ↓
First-run check
 ↓
Language selector (only when required)
 ↓
Usable desktop
```

## 2.2 Ordinary user capabilities

The finished SB Desktop must support, through GUI where appropriate:

- launching and closing applications;
- window movement, resizing, minimizing, maximizing and switching;
- keyboard and mouse input;
- file creation and organization;
- copy, move, rename and safe deletion;
- network configuration;
- language configuration;
- keyboard layout configuration;
- display configuration;
- audio configuration;
- account configuration;
- privacy and security configuration;
- storage management;
- software search, install, remove and update;
- update progress and rollback information;
- diagnostics and support-report generation;
- recovery actions;
- shutdown, reboot and sleep/standby where hardware supports them;
- Terminal access.

## 2.3 Advanced-user capabilities

The same OS must expose:

- package management from CLI;
- process inspection;
- logs;
- network diagnostics;
- storage diagnostics;
- hardware/device information;
- developer tools;
- recovery commands;
- performance information;
- detailed configuration;
- machine-readable diagnostic output.

---

# 3. DESIGN GOALS AND NON-GOALS

## 3.1 Goals

### G-01 — Lightweight base

Do not ship optional applications and services in the minimum image merely because they may be useful to someone.

### G-02 — Fast startup

Avoid unnecessary initialization, polling and background services.

### G-03 — Fast optional installation

Package retrieval must be compact, resumable, verified and efficient.

### G-04 — Detailed settings

Users should have meaningful control without needing to edit obscure files for common configuration.

### G-05 — Explainable failures

A failure should communicate its meaning and expose technical detail when useful.

### G-06 — Broad hardware support

The architecture must allow multiple drivers and hardware backends without changing applications for each vendor.

### G-07 — Minecraft readiness

Minecraft and Minecraft Server workloads should be first-class performance targets, while the underlying OS remains a general-purpose desktop OS.

### G-08 — CLI parity where appropriate

GUI and CLI should call the same system services rather than implementing two contradictory versions of the same logic.

## 3.2 Non-goals during the current phase

- separate CUI-only edition;
- dozens of premature hardware-specific ISO images;
- vendor-specific kernel forks;
- features that are merely visual mockups;
- replacing stability with benchmark-only optimizations.

---

# PART II — COMPLETE SYSTEM ARCHITECTURE

# 4. GLOBAL LAYER MODEL

```text
Firmware
  ↓
Boot environment
  ↓
Architecture-specific early kernel
  ↓
CPU / Memory / Interrupt foundation
  ↓
Kernel core
  ↓
Device and I/O subsystems
  ↓
Filesystem / VFS
  ↓
Userspace init / service manager
  ↓
System services
  ↓
Display / input services
  ↓
GUI platform
  ↓
Desktop shell
  ↓
System applications
  ↓
Optional applications / Store components
```

The dependency direction is intentionally one-way wherever practical.

## 4.1 Kernel must not depend on GUI

The kernel cannot require:

- a window system;
- a font engine;
- a desktop shell;
- the package store;
- a graphical language popup.

Kernel startup and recovery must remain possible without userspace GUI.

## 4.2 GUI must not access hardware directly

GUI processes communicate with display/input/device services using defined interfaces.

## 4.3 One source of truth

The same setting, package state, device state or account state must not be represented by multiple independent databases.

---

# 5. COMPLETE BOOT ARCHITECTURE

## 5.1 Firmware

SB should support a firmware abstraction capable of handling BIOS and UEFI-based machines as the project expands.

Firmware-specific details must remain below the generic boot/platform interface.

## 5.2 Bootloader responsibilities

The bootloader must:

- load the kernel;
- provide required boot modules;
- establish the CPU execution environment expected by the kernel;
- establish or pass required page-table information;
- establish an early stack;
- pass boot protocol metadata;
- identify relevant loaded-image and reserved-memory ranges;
- transfer control to the kernel entry point.

It must not initialize the desktop.

## 5.3 Boot information ownership

All externally supplied structures have an explicit lifecycle:

```text
received
 ↓
validated
 ↓
protected
 ↓
parsed
 ↓
copied/normalized when ownership requires it
 ↓
consumers registered
 ↓
original storage released only when no consumer needs it
```

This applies to Multiboot2 structures, firmware tables, memory maps, ACPI data and boot modules.

## 5.4 Boot states

The kernel should expose an internal state machine conceptually similar to:

```text
RESET
EARLY_CPU
BOOT_INFO_READY
EARLY_CONSOLE_READY
MEMORY_BOOTSTRAP
MEMORY_READY
INTERRUPTS_READY
SCHEDULER_READY
USERSpace_READY
STORAGE_READY
DISPLAY_READY
DESKTOP_READY
RECOVERY
PANIC
```

State transitions must be monotonic except for explicitly defined recovery transitions.

## 5.5 Boot failure policy

Boot failures are classified by earliest safe recovery boundary:

1. failure before diagnostic console;
2. failure after serial/early console but before graphics;
3. failure after graphics initialization;
4. userspace/service failure;
5. desktop/application failure.

The system should never require a graphical mechanism to diagnose a failure that occurs before graphics exists.

---

# 6. CPU AND ARCHITECTURE LAYER

## 6.1 Initial architecture

Initial production target:

**x86_64**

Initial development/test environment:

**QEMU x86_64**

## 6.2 Implementation languages

Kernel:

- C as the primary systems language;
- x86_64 Assembly for boot entry, low-level context switching, interrupt/syscall entry and hardware-specific primitives where necessary.

Supporting implementation may use:

- linker scripts;
- shell/build scripts;
- CI YAML;
- host-side test utilities.

## 6.3 Architecture abstraction

Architecture-dependent code must be isolated so that future architectures can be considered without rewriting high-level kernel services.

Potential future architecture ports are not required for the first product, but the boundary must be clean.

## 6.4 CPU feature detection

The kernel must detect and record supported CPU capabilities rather than assuming that every x86_64 machine has identical features.

Potential capabilities include:

- APIC;
- invariant TSC where available;
- NX;
- syscall/sysret support;
- SMEP/SMAP where available;
- PCID where useful;
- virtualization extensions;
- vector instruction capabilities;
- other capabilities used by optional optimizations.

An optimization must have a fallback if the feature is optional.

---

# 7. PHYSICAL MEMORY MANAGER (PMM)

## 7.1 Purpose

PMM owns physical page allocation and reservation.

## 7.2 Memory ownership states

Every physical page must conceptually be in one of these states:

```text
UNKNOWN
RESERVED
FREE
ALLOCATED
RECLAIMABLE
FIRMWARE
DEVICE
```

Transitions must be explicit.

## 7.3 Protected ranges

The PMM must never return a page that belongs to:

- kernel image;
- active boot stack;
- active page tables;
- PMM metadata;
- active Multiboot structures;
- active UEFI structures;
- active ACPI tables;
- loaded boot modules still in use;
- device/firmware reserved regions;
- other explicitly owned allocations.

## 7.4 Bootstrap mode

A bounded bootstrap allocator may be used before full memory-map integration is operational.

Bootstrap mode must be clearly identified in the code and must not be mistaken for the final memory-management policy.

## 7.5 Final memory-map integration

The final PMM must consume a normalized platform memory map.

Memory-map entries should be converted into internal types before allocator policy is applied.

## 7.6 PMM invariants

- page-aligned allocations;
- no allocation from reserved memory;
- no duplicate ownership;
- no double free;
- invalid addresses rejected;
- exhaustion explicitly reported;
- metadata memory accounted for;
- reinitialization cannot destroy live allocation ownership.

## 7.7 PMM debugging

Low-level diagnostics must not recursively depend on the PMM being debugged.

Boot diagnostics from PMM must use a minimal logging path that does not allocate from PMM or wait on user-facing services.

---

# 8. VIRTUAL MEMORY MANAGER (VMM)

## 8.1 Responsibilities

- page-table construction;
- mapping;
- unmapping;
- permission flags;
- address-space creation;
- address-space destruction;
- kernel/user separation;
- page-fault integration;
- TLB management.

## 8.2 Required protection model

Userspace must not be able to:

- write kernel-only memory;
- execute explicitly non-executable memory where NX is enforced;
- access another process's private mappings;
- alter page tables without privileged APIs.

## 8.3 Mapping lifecycle

```text
request mapping
 ↓
validate alignment/range
 ↓
allocate page-table structures if needed
 ↓
validate physical ownership
 ↓
install mapping
 ↓
invalidate/update translation state
 ↓
return handle/result
```

Unmapping must correctly update ownership and TLB state.

## 8.4 Large pages

Support for large pages is optional initially. If added, the API must distinguish page size and alignment and must not make 4 KiB pages impossible.

---

# 9. KERNEL HEAP

## 9.1 Responsibilities

Provide dynamic kernel allocations on top of PMM/VMM.

## 9.2 Requirements

- defined alignment;
- allocation failure path;
- zero-size behavior;
- integer-overflow checks;
- no hidden dependency on GUI/logging;
- correct freeing;
- fragmentation measurement;
- optional per-CPU caches only after correctness exists.

## 9.3 Allocation classes

Eventually separate:

- page allocations;
- physically contiguous allocations;
- ordinary heap objects;
- DMA-safe allocations where required.

A driver must explicitly request the correct class instead of assuming every allocation is physically contiguous.

---

# 10. GDT / TSS / IDT / EXCEPTIONS

## 10.1 GDT

Provide defined kernel and user execution/data contexts.

## 10.2 TSS

Provide privilege-transition stack support and architecture-required state.

Each CPU that needs a distinct interrupt stack must have its own state.

## 10.3 IDT

All architecture exceptions must have defined entry paths.

## 10.4 Exception handling

For each exception:

- capture relevant CPU state;
- identify process/thread;
- determine recoverability;
- deliver to userspace when safe;
- otherwise enter kernel diagnostic path.

## 10.5 Page fault handling

Page faults must distinguish at least:

- valid demand allocation/page-in path;
- copy-on-write if implemented later;
- guard-page violation;
- permission violation;
- unmapped address;
- kernel fault.

---

# 11. INTERRUPTS, APIC, PIC AND TIMERS

## 11.1 Timer architecture

Use a layered timer interface rather than exposing PIT details to the scheduler.

Initial development may use PIT.

Later the kernel may select among:

- PIT;
- HPET where appropriate;
- local APIC timer;
- TSC-derived timing where safe;
- platform timer facilities.

The selected timer source must expose one monotonic kernel timebase.

## 11.2 Migration rule

Timer source selection must be explicit:

```text
Early boot timer
 ↓
Basic interrupt/tick operation
 ↓
APIC/advanced timer initialization
 ↓
Clocksource validation
 ↓
Scheduler clock migration
```

The scheduler must not implicitly change timer units during migration.

## 11.3 PIC/APIC

Legacy PIC may be supported during early bring-up, but final interrupt routing should support modern APIC-based systems.

---

# 12. ACPI AND PLATFORM DISCOVERY

ACPI handling must be separated from generic device logic.

Responsibilities include discovering:

- processor topology;
- interrupt routing information;
- power-management capabilities;
- relevant platform tables;
- device descriptions exposed through ACPI.

ACPI data has a lifecycle and remains protected while referenced.

Malformed ACPI must not make the kernel blindly trust corrupted tables.

---

# 13. SMP / MULTI-CORE

## 13.1 Goals

Support multiple logical CPUs after single-CPU boot is stable.

## 13.2 CPU-local data

Each CPU may require private state including:

- current thread;
- scheduler run state;
- interrupt nesting;
- per-CPU logger buffer;
- timer data;
- architecture state;
- temporary bootstrap state.

## 13.3 Startup

```text
bootstrap CPU
 ↓
discover topology
 ↓
validate CPU set
 ↓
prepare per-CPU structures
 ↓
start application CPUs
 ↓
initialize local interrupts/timers
 ↓
join scheduler
```

Failure to start one optional CPU should not automatically crash the entire machine.

---

# 14. SYNCHRONIZATION

Provide primitives such as:

- spinlock;
- mutex;
- semaphore where justified;
- condition/event mechanism;
- interrupt-safe locking variants;
- atomic operations.

## 14.1 Lock rules

Each lock must have:

- ownership semantics;
- context restrictions;
- interrupt restrictions;
- lock-order policy;
- expected hold time;
- debug detection where feasible.

No lock may be acquired in an interrupt context if the implementation can block.

## 14.2 Logging rule

Logging from low-level code must not require locks that can be held by the caller.

Preferred path:

```text
critical kernel path
 ↓
non-blocking event/log record
 ↓
per-CPU or lock-minimized ring buffer
 ↓
logger service
 ↓
serial/file/GUI destinations
```

---

# PART III — EXECUTION MODEL

# 15. SCHEDULER

## 15.1 Objective

Provide responsive execution while preserving deterministic and explainable behavior.

## 15.2 Initial scheduler

A simple round-robin scheduler is acceptable during bring-up.

## 15.3 Future scheduler

The architecture should allow policies for:

- interactive desktop responsiveness;
- background tasks;
- I/O-bound workloads;
- gaming workloads;
- Minecraft server workloads;
- CPU-intensive tasks.

Minecraft-oriented policy must remain a policy layer rather than contaminating generic process management.

## 15.4 Scheduler states

```text
NEW
READY
RUNNING
SLEEPING
BLOCKED_IO
STOPPED
EXITED
ZOMBIE
```

Transitions must release resources appropriately.

## 15.5 No busy loops

A task waiting for an event should block/sleep rather than consume an entire CPU by polling.

---

# 16. THREADS AND PROCESSES

## 16.1 Thread

A thread owns CPU execution state and scheduling metadata.

## 16.2 Process

A process owns an address space and resource namespace.

## 16.3 Process lifecycle

```text
create
 ↓
load/initialize
 ↓
runnable
 ↓
running
 ↓
exit
 ↓
resource cleanup
 ↓
reaped
```

## 16.4 Resource accounting

At process level, eventually track:

- memory;
- CPU time;
- open handles;
- file descriptors;
- sockets;
- child processes;
- package/app identity;
- security context.

---

# 17. IPC

Provide controlled mechanisms such as:

- pipes;
- queues;
- shared memory;
- event objects;
- local sockets;
- request/response service interfaces.

IPC must enforce ownership and permissions.

Shared memory must not implicitly grant arbitrary access to unrelated processes.

---

# 18. SYSCALL ABI

## 18.1 General rule

The userspace ABI must be versioned and validated.

## 18.2 Categories

Eventually include:

- process/thread operations;
- memory mapping;
- file descriptors;
- filesystem operations;
- clocks/timers;
- IPC;
- sockets/networking;
- device/service operations;
- security/account operations where exposed.

## 18.3 ABI safety

At every syscall boundary validate:

- pointer ranges;
- buffer lengths;
- integer overflow;
- handle validity;
- permission;
- object ownership.

A userspace pointer must never be trusted simply because it is non-null.

---

# 19. ELF / PROGRAM LOADING

## 19.1 Loader

The loader must:

- validate ELF identity and class;
- validate architecture;
- validate program-header bounds;
- validate segment sizes;
- check alignment;
- allocate/map required pages;
- apply permissions;
- construct user stack;
- determine entry point;
- reject malformed or impossible binaries.

## 19.2 No implicit trust of modules

A boot module claiming to be an executable must still be validated as input.

---

# 20. USERSPACE INIT AND SERVICE MANAGER

## 20.1 Init

The first userspace process is responsible for starting the minimal set of system services needed for a usable desktop.

## 20.2 Service states

```text
DECLARED
STARTING
READY
FAILED
RESTARTING
STOPPING
STOPPED
DISABLED
```

## 20.3 Service policy

Services should start only when needed unless they are fundamental to the desktop.

Optional software installation must not automatically create always-running background services unless the user or system policy explicitly requires it.

---

# PART IV — STORAGE, DATA AND CONFIGURATION

# 21. STORAGE STACK

```text
Physical device
 ↓
Controller driver
 ↓
Block layer
 ↓
Partition layer
 ↓
Filesystem driver
 ↓
VFS
 ↓
Path/file APIs
 ↓
Applications
```

## 21.1 Device classes

Initial direction:

- SATA/ATA-compatible storage;
- NVMe;
- USB storage;
- virtual disks in QEMU.

## 21.2 NVMe

NVMe support must use the controller's proper submission/completion queue model and DMA requirements. The architecture must not assume that a storage buffer is automatically DMA-safe.

Current NVMe standards are maintained by NVM Express; the project should implement against a selected, documented specification revision and avoid embedding unexplained vendor-specific behavior. citeturn342371search0turn342371search1

---

# 22. FILESYSTEM AND VFS

## 22.1 VFS responsibilities

Provide common operations independent of the underlying filesystem:

- open;
- close;
- read;
- write;
- seek;
- stat;
- create;
- mkdir;
- rename;
- unlink;
- directory enumeration;
- permissions;
- timestamps.

## 22.2 Filesystem responsibilities

A filesystem implementation owns its on-disk metadata and consistency rules.

The VFS must not make filesystem-specific assumptions.

## 22.3 Atomic configuration writes

When writing critical configuration:

```text
create new version
 ↓
write complete content
 ↓
flush/sync according to storage semantics
 ↓
atomically switch active reference
 ↓
retain rollback copy if policy requires
```

## 22.4 User-data safety

Normal system cleanup must never delete arbitrary user files.

---

# 23. DATA CLASSIFICATION

Every persistent path should belong to a known class:

```text
SYSTEM
USER
CACHE
TEMP
LOG
PACKAGE_STATE
UPDATE_STAGING
RECOVERY
SECRETS
FIRMWARE/DEVICE
```

Each class has separate lifecycle and cleanup rules.

## 23.1 SYSTEM

Core OS configuration and runtime state.

## 23.2 USER

Documents, application data and user-created content.

Never automatically delete without an explicit user action or documented recovery policy.

## 23.3 CACHE

Re-creatable downloaded or generated data.

Can be cleaned according to policy.

## 23.4 TEMP

Short-lived data.

Must have ownership and expiration rules.

## 23.5 LOG

Diagnostic records with retention limits.

## 23.6 UPDATE_STAGING

Temporary package/update data that can be discarded or rolled back after transaction completion.

## 23.7 SECRETS

Credentials, keys and authentication material.

Must be handled separately from ordinary logs and configuration exports.

---

# 24. CONFIGURATION SERVICE

All system settings should eventually be accessed through one configuration service/API rather than every application editing independent copies.

Each setting requires:

- stable ID;
- value type;
- default;
- valid range/schema;
- current value;
- persistence method;
- access policy;
- owning subsystem;
- migration version.

## 24.1 Configuration transactions

Grouped settings changes should support:

```text
read current
 ↓
validate proposal
 ↓
apply in memory
 ↓
persist transactionally
 ↓
notify observers
```

A failed setting change should not leave half the settings applied.

---

# 25. TIME, CLOCKS, REGION AND LOCALE

Separate:

- monotonic kernel time;
- wall-clock time;
- RTC synchronization;
- timezone;
- locale;
- date/time formatting.

Applications must not use wall-clock time for timeout logic where monotonic time is required.

---

# PART V — DEVICES AND HARDWARE

# 26. PCI / PCI EXPRESS

PCI is the base discovery mechanism for many devices.

The implementation must discover device identity and capabilities before loading the correct driver.

PCI device access must obey platform and bus constraints; the driver layer must not assume that configuration space values are always identical across generations.

PCI-SIG maintains the PCI Express specifications and revisions; implementation work should target explicitly documented revisions/features rather than assuming every optional capability exists. citeturn511015search4turn511015search11

## 26.1 Device discovery result

A discovered device should eventually expose:

- bus/device/function;
- vendor/device ID;
- class/subclass/prog-if;
- BARs/resources;
- interrupt mechanism;
- capabilities;
- attached driver;
- status.

---

# 27. DRIVER FRAMEWORK

## 27.1 Driver lifecycle

```text
DISCOVERED
 ↓
MATCHED
 ↓
PROBING
 ↓
INITIALIZED
 ↓
BOUND
 ↓
ACTIVE
 ↓
STOPPING
 ↓
REMOVED
```

## 27.2 Driver isolation

A faulty optional driver should fail as locally as safely possible.

Driver ownership must be clear:

- which device it owns;
- what resources it allocated;
- what interrupts it registered;
- what DMA buffers it owns;
- what service endpoints it exposes.

## 27.3 Missing driver behavior

If no driver exists, the device should appear in diagnostics as unsupported rather than silently corrupting the system.

If a generic fallback exists, use it.

---

# 28. USB

The USB architecture should consist of:

```text
USB host controller
 ↓
USB core
 ↓
device/enumeration layer
 ↓
class drivers
 ↓
input/storage/audio/etc.
```

Support should expand incrementally from foundational USB 2.0 functionality toward newer controllers and USB4-related capabilities as hardware support matures. USB-IF publishes the relevant USB and USB Type-C/UCSI specifications and compliance material. citeturn342371search3turn342371search4

Hotplug events must be explicit and safe.

---

# 29. INPUT

Input subsystem must abstract:

- keyboard;
- mouse;
- touch if later supported;
- buttons/hotkeys;
- device addition/removal;
- focus routing.

Raw hardware events are converted into normalized input events before GUI applications consume them.

---

# 30. DISPLAY AND GRAPHICS

## 30.1 Display layers

```text
GPU/display hardware
 ↓
Kernel/driver abstraction
 ↓
Display service
 ↓
Framebuffer / rendering backend
 ↓
Compositor
 ↓
Window system
 ↓
GUI toolkit
 ↓
Applications
```

## 30.2 Generic fallback

A generic framebuffer path should exist when technically possible so that lack of an accelerated vendor driver does not automatically make the desktop unusable.

## 30.3 GPU progression

Hardware support should progress approximately as:

1. generic framebuffer;
2. stable display modes;
3. basic acceleration abstraction;
4. vendor-specific drivers;
5. modern graphics APIs where justified;
6. advanced compute/graphics capabilities.

Vulkan support, if adopted, should be implemented as a userspace API/backend architecture and not confused with the low-level device driver itself. Vulkan is defined as a C99 graphics/compute API by Khronos. citeturn342371search60

---

# 31. GPU COMPATIBILITY TARGETS

Future compatibility includes:

- Intel integrated/discrete graphics;
- AMD Radeon families;
- NVIDIA consumer GPUs;
- NVIDIA RTX families;
- NVIDIA professional/data-center GPUs;
- other high-performance accelerators as the driver architecture permits.

Important distinction:

**“The PCI device is detected” is not “the GPU is supported.”**

A release claiming GPU support must demonstrate at least:

- correct device initialization;
- display output;
- memory management;
- required power/state transitions;
- graphics workload execution appropriate to the claimed support level;
- stability tests.

---

# 32. AUDIO

Audio should be a service-backed subsystem with device drivers underneath.

Responsibilities:

- device discovery;
- playback/capture;
- stream format negotiation;
- volume/mute;
- device selection;
- application stream management;
- failure recovery.

No GUI application should speak directly to a hardware codec.

---

# 33. POWER MANAGEMENT

Eventually support:

- shutdown;
- restart;
- sleep;
- hibernate if technically safe;
- battery information;
- thermal information;
- idle CPU states;
- device power states.

Power transitions must be state-machine driven and recover from partial device failures where possible.

---

# PART VI — GUI AND DESKTOP

# 34. GUI ARCHITECTURE

## 34.1 Layers

```text
Display backend
 ↓
Compositor
 ↓
Window system
 ↓
GUI toolkit
 ↓
Desktop shell
 ↓
Applications
```

## 34.2 Core GUI primitives

- window/surface;
- text;
- font;
- label;
- button;
- checkbox;
- radio selection;
- select/drop-down;
- text field;
- list;
- tree;
- menu;
- dialog;
- notification;
- progress indicator;
- scrollbar;
- tab;
- toolbar;
- context menu.

## 34.3 Event model

The GUI is event-driven.

Events include:

- pointer move;
- pointer button;
- keyboard press/release;
- text input;
- focus change;
- resize;
- window state change;
- timer/event callbacks;
- device change;
- system notifications.

## 34.4 Rendering policy

UI rendering must not require unnecessary full-screen redraws when incremental updates are possible.

The toolkit should eventually support damage tracking and composition optimizations.

---

# 35. DESKTOP SHELL

The desktop shell provides the default user environment.

Minimum capabilities:

- desktop/workspace surface;
- application launcher;
- task/window switching;
- system status area;
- notifications;
- network status;
- audio status;
- clock;
- settings access;
- file manager access;
- terminal access;
- power controls.

Installed optional software does not automatically become a permanent resident background process merely because it is installed.

---

# 36. FIRST BOOT — EXACT REQUIRED EXPERIENCE

This is a core SB requirement.

## 36.1 Trigger

Show first-run UI when a valid persistent language configuration does not exist.

## 36.2 What must appear

After the graphical desktop is available, show a normal GUI popup such as:

```text
Welcome to SuiraBox

Language
[ English ▼ ]

[ Continue ]
```

The language field is a real graphical select/drop-down control.

It is not a terminal prompt.

## 36.3 Initial language choices

- 日本語
- English
- 中文
- Español

## 36.4 Keyboard layout dependency

Language and keyboard layout are distinct settings.

The first-run system must therefore implement this safe ordering:

```text
Choose display language
 ↓
Choose or confirm keyboard layout
 ↓
Apply input configuration
 ↓
Continue with any text-entry-dependent setup
```

A sensible default may be inferred from locale/hardware, but the user must be able to override it.

Never assume that language automatically determines physical keyboard layout.

## 36.5 First-run persistence

After the user confirms:

1. validate language ID;
2. validate keyboard-layout ID;
3. write both atomically;
4. activate localization/input services;
5. mark first-run complete;
6. continue to normal desktop.

## 36.6 Corrupt first-run state

If the configuration later becomes invalid:

- detect it;
- preserve diagnostics;
- repair/rebuild the configuration when possible;
- otherwise reopen a safe setup path;
- never create an infinite setup/reboot loop.

---

# 37. LOCALIZATION / INTERNATIONALIZATION

## 37.1 Initial languages

- Japanese
- English
- Chinese
- Spanish

## 37.2 String architecture

Every user-visible string uses a stable identifier.

Example:

```text
ui.firstboot.title
ui.firstboot.language
ui.firstboot.keyboard_layout
ui.firstboot.continue
error.network.timeout
panic.kernel.page_fault
settings.display.refresh_rate
```

Applications must not hard-code translated strings throughout the program.

## 37.3 Locale versus language

Maintain separately:

- interface language;
- locale formatting;
- keyboard layout;
- input method;
- timezone;
- region.

## 37.4 Unicode

The UI stack must support Unicode end-to-end.

The text path must account for:

- variable-width glyphs;
- combining characters;
- fallback fonts;
- right-to-left rendering capability as a future extension;
- line breaking;
- normalization where relevant.

## 37.5 IME

Japanese and Chinese input require an input-method architecture rather than assuming that one keyboard event equals one final Unicode character.

IME integration belongs above raw keyboard drivers and below text fields/applications.

## 37.6 Missing translation fallback

If a translation is missing:

1. use fallback language;
2. never display an uninitialized string ID;
3. log the missing translation for developers;
4. do not block the application merely because a translation is missing.

---

# 38. SETTINGS

## 38.1 Main categories

- System
- Appearance
- Display
- Sound
- Network
- Keyboard & Mouse
- Language & Region
- Storage
- Applications
- Accounts
- Privacy & Security
- Updates
- Performance
- Developer
- Recovery
- Accessibility

## 38.2 Setting presentation

The UI must be readable even when many detailed controls exist.

Preferred hierarchy:

```text
Category
 ↓
Section
 ↓
Setting
 ↓
Current value
 ↓
Description
 ↓
Advanced details if needed
```

## 38.3 Advanced options

Detailed controls can be hidden under Advanced sections where exposing every option simultaneously would reduce usability.

The setting still exists; only its presentation is simplified.

---

# 39. ACCESSIBILITY

The GUI must eventually consider:

- keyboard navigation;
- focus visibility;
- scalable text;
- high-contrast modes;
- reduced animation preference;
- screen-reader integration boundary;
- color-independent status communication;
- pointer-size options.

Accessibility features must not require rewriting the entire application framework later, so accessibility metadata belongs in the toolkit design from the beginning.

---

# 40. CLIPBOARD, NOTIFICATIONS AND WORKSPACES

## Clipboard

Provide secure copy/paste semantics across applications with ownership/timeouts as needed.

## Notifications

Notifications must:

- have severity/category;
- be dismissible where appropriate;
- avoid exposing secrets;
- avoid overwhelming the user with repeated identical failures.

## Workspaces

The desktop may support multiple workspaces, but workspace state must not become an unnecessary permanent memory cost when unused.

---

# 41. FILE MANAGER

The base desktop should provide a file manager capable of:

- navigation;
- search;
- folder creation;
- file copy/move;
- rename;
- delete with appropriate confirmation/recovery behavior;
- metadata display;
- storage-device visibility;
- permission-aware operations.

Large operations must not freeze the GUI thread. Progress belongs in a task/service mechanism.

---

# PART VII — NETWORKING

# 42. NETWORK ARCHITECTURE

```text
NIC driver
 ↓
Network device abstraction
 ↓
Link layer
 ↓
IPv4/IPv6
 ↓
ARP/Neighbor Discovery
 ↓
Routing
 ↓
UDP/TCP
 ↓
DNS/DHCP and other services
 ↓
Socket API
 ↓
Network manager
 ↓
GUI / CLI applications
```

## 42.1 Initial targets

- Ethernet;
- loopback;
- IPv4;
- IPv6;
- DHCP;
- DNS;
- sockets.

## 42.2 Later capabilities

- Wi-Fi;
- firewall;
- VLAN;
- VPN interfaces where compatible;
- advanced routing;
- additional network diagnostics.

## 42.3 Failure behavior

Every network request must have:

- timeout;
- cancellation semantics;
- connection/error result;
- retry policy where appropriate;
- offline behavior.

The GUI must never freeze indefinitely while a network request waits.

---

# 43. NETWORK MANAGER

The Network Manager is the single high-level authority for normal connection state.

It tracks:

- interfaces;
- addresses;
- routes;
- DNS settings;
- DHCP state;
- link state;
- Wi-Fi state when supported;
- connected/disconnected/degraded status.

GUI and CLI query/change the same service.

---

# PART VIII — TERMINAL AND SOFTWARE DISTRIBUTION

# 44. TERMINAL / SHELL

SB Desktop includes a real terminal.

## 44.1 Required capabilities

- command execution;
- arguments;
- environment variables;
- pipelines;
- redirection;
- working directories;
- signals/job control where implemented;
- scripting foundation;
- filesystem tools;
- network tools;
- process tools;
- package tools;
- diagnostic tools;
- recovery tools.

## 44.2 CLI/GUI parity

Where an operation has a GUI and CLI form, both should call the same underlying system service or library.

Example:

```text
SB Store GUI ─┐
              ├─> Package Service ─> Transaction Engine
sbpkg CLI ────┘
```

This prevents the GUI and CLI from having contradictory package databases.

---

# 45. PACKAGE SYSTEM

## 45.1 Package lifecycle

```text
search
 ↓
select version
 ↓
resolve dependencies
 ↓
obtain metadata
 ↓
verify repository metadata
 ↓
download package(s)
 ↓
verify package integrity/authenticity
 ↓
stage
 ↓
preflight
 ↓
install transaction
 ↓
register state
 ↓
activate
 ↓
cleanup temporary data
```

## 45.2 Package metadata

Every package should eventually contain:

- package ID;
- name;
- version;
- architecture compatibility;
- dependencies;
- conflicts;
- provides;
- files;
- entry points where applicable;
- service declarations where applicable;
- permissions/capabilities;
- integrity hashes;
- signature/reference information;
- localization metadata where applicable.

## 45.3 Transactions

Install/remove/update operations must be transactional.

If an operation fails partway through:

- preserve known-good state;
- undo partial modifications where possible;
- mark recovery state;
- provide an actionable error.

## 45.4 Package concurrency

The package database and filesystem transaction layer must use an exclusive package transaction lock.

If CLI and GUI attempt concurrent modification:

```text
first operation owns transaction lock
second operation sees BUSY
 ↓
GUI: show “Another package operation is in progress” + progress/status
CLI: return machine-readable busy result or wait only when explicitly requested
```

Read-only operations may use appropriate shared access if safe.

## 45.5 SB Store

SB Store is a GUI frontend to the package ecosystem.

It must not create a separate package database.

The Store provides:

- search;
- categories;
- package details;
- compatibility status;
- install;
- remove;
- update;
- progress;
- cancellation where safe;
- dependency explanation;
- disk-space requirement;
- rollback/error explanation.

---

# 46. FAST AND LIGHTWEIGHT DOWNLOADS

Because minimal base installation is a core SB principle, package retrieval must be optimized.

## Required mechanisms where practical

- compressed metadata;
- compressed packages;
- content hashing;
- local caching;
- resumable downloads;
- range requests where supported;
- connection reuse;
- parallel downloads for independent large files where it improves performance;
- mirrors/CDN later;
- deduplication where practical.

Integrity always has priority over raw download speed.

The installer must not redownload a valid cached object unnecessarily.

---

# 47. OPTIONAL COMPONENT MODEL

Potential optional components include:

- extra language packs;
- fonts;
- applications;
- development tools;
- multimedia support;
- GPU-related userland components;
- JVM/runtime components;
- Minecraft support tooling;
- server tooling;
- themes;
- desktop extensions.

Optional components must declare dependencies and whether they require a reboot or service restart.

---

# PART IX — SECURITY

# 48. SECURITY MODEL

Security is enforced at actual privilege boundaries.

## 48.1 Core isolation

- kernel memory protected from userspace;
- process address spaces isolated;
- syscall input validated;
- privileged device access controlled;
- package authenticity verified;
- update authenticity verified;
- secrets separated from ordinary logs.

## 48.2 Capability/permission direction

The future security model should distinguish:

- user identity;
- process identity;
- application identity;
- resource permission;
- elevated administrative capability.

## 48.3 Application isolation

Optional applications should run with minimum required privileges where technically practical.

Potential future isolation levels:

```text
normal process
restricted process
sandboxed application
privileged service
kernel driver
```

A GUI feature must not gain kernel privileges merely because its implementation is convenient.

---

# 49. PACKAGE AND SUPPLY-CHAIN SECURITY

The package ecosystem must defend against:

- tampered packages;
- corrupted downloads;
- stale metadata;
- replay of outdated metadata where applicable;
- dependency confusion;
- malicious package names pretending to be official;
- compromised mirrors;
- partially written packages;
- unsigned or unverifiable metadata.

Official package sources must be distinguishable from third-party sources.

---

# 50. SECRETS

Secrets must never be copied into:

- ordinary logs;
- support reports;
- crash screens;
- public diagnostic exports;
- package metadata;
- casual configuration dumps.

Future secret storage must separate credentials/keys from ordinary configuration.

---

# PART X — ERRORS, DIAGNOSTICS AND RECOVERY

# 51. ERROR CLASSIFICATION

SB uses an explicit severity hierarchy.

### Level A — Informational

No failure. Example: package download complete.

### Level B — Recoverable warning

A component had trouble, but normal operation continues.

### Level C — User-facing error

An operation failed but the OS remains usable.

### Level D — Service failure

A system service failed and requires restart/recovery.

### Level E — System degradation

A larger subsystem is unavailable but the machine may remain usable.

### Level F — Kernel emergency

Safe continued execution cannot be guaranteed.

### Level G — Early-boot / recovery integrity emergency

The normal recovery or display path cannot be trusted.

---

# 52. NORMAL ERROR UX

A normal error dialog should contain, as appropriate:

- clear human-readable title;
- concise explanation;
- affected action/component;
- current usability state;
- recommended next step;
- Error ID;
- expandable details;
- retry/recover option;
- support-report option for serious issues.

Example conceptual structure:

```text
Something went wrong

SB could not connect to the selected network.

The desktop is still working normally.

[ Retry ] [ Network Settings ] [ Details ]

Error ID: NET-...
```

Do not show a terrifying kernel-style screen for ordinary application errors.

---

# 53. BSOD

## 53.1 Meaning

BSOD means:

> **The system was operating, but continuing is no longer safe or reliable.**

It is a kernel/system stop state.

## 53.2 Causes

Examples include:

- unrecoverable kernel exception;
- memory-management invariant corruption;
- scheduler/kernel-state corruption;
- security boundary violation inside the kernel;
- critical infrastructure failure.

## 53.3 Required technical information when available

- stable error ID;
- error class;
- exception/vector;
- CPU error code;
- instruction pointer;
- stack pointer;
- CPU/core ID;
- process/thread ID;
- subsystem;
- kernel build/version;
- boot/session ID;
- crash-dump status;
- recovery state.

## 53.4 User-facing design

The primary message should be understandable.

Technical diagnostics should be expandable or separately accessible.

The screen should state what happens next, such as automatic restart, safe recovery attempt or manual recovery requirement.

---

# 54. RSOD

## 54.1 Meaning

RSOD means:

> **The normal display, recovery path, or critical early-boot/system-integrity path cannot be trusted.**

## 54.2 Allowed use

Only for exceptionally severe states such as:

- graphics/recovery path itself corrupted;
- early boot integrity failure;
- critical system image validation failure where normal diagnostics cannot safely operate;
- recovery environment unavailable or untrusted.

## 54.3 Forbidden use

Do not use RSOD because:

- an application crashed;
- a file failed to open;
- a package failed to install;
- the network disconnected;
- the desktop service restarted.

---

# 55. CRASH DUMPS

Crash dumps must be:

- versioned;
- bounded in size;
- privacy-aware;
- resilient against recursion/failure;
- identifiable by crash/session ID.

A crash dump must never rely on the fully functioning GUI.

---

# 56. LOW-LEVEL LOGGING

## 56.1 Logging levels

- TRACE
- DEBUG
- INFO
- NOTICE
- WARNING
- ERROR
- CRITICAL
- PANIC

## 56.2 Log record

At minimum:

- timestamp;
- CPU/core;
- severity;
- component;
- event/error ID;
- session/boot ID;
- message/structured fields.

## 56.3 Recursion rule

A logger used while the allocator, scheduler, interrupt controller or logger itself is broken must have a minimal fallback path that does not call the failing subsystem recursively.

Preferred architecture:

```text
lowest-level event
 ↓
static/minimal record
 ↓
serial / emergency buffer
```

Then normal logging can take over later.

---

# 57. SUPPORT REPORT

A support report should be available from Settings/Recovery.

It may contain:

- SB version/build;
- hardware inventory;
- driver states;
- service states;
- recent relevant logs;
- package state;
- storage health information where available;
- network diagnostic state where appropriate;
- crash IDs;
- configuration summary.

It must exclude known secret classes.

It should be exportable as a stable, documented format.

---

# 58. RECOVERY ARCHITECTURE

Recovery must prefer the smallest safe repair.

Preferred sequence:

```text
application restart
 ↓
service restart
 ↓
subsystem reinitialization
 ↓
package rollback
 ↓
recovery boot
 ↓
full system stop
```

The OS must not repeatedly restart a permanently failing component without a backoff/disable condition.

---

# 59. RECOVERY MODE

Recovery mode should eventually provide:

- filesystem checks;
- package rollback;
- configuration rollback;
- failed-service disablement;
- log export;
- update rollback;
- safe driver disablement;
- basic network diagnostics;
- system integrity checks.

Recovery mode must minimize dependencies on the normal desktop.

---

# PART XI — UPDATE, BACKUP, CLEANUP

# 60. UPDATE SYSTEM

Target transaction:

```text
check metadata
 ↓
verify metadata
 ↓
resolve dependencies
 ↓
download
 ↓
verify content
 ↓
stage
 ↓
preflight
 ↓
commit/activate atomically
 ↓
update state
 ↓
validate next boot
 ↓
retain rollback where policy requires
```

## 60.1 Interrupted update

If power is lost during update, the machine must boot into a valid previous or recovery state rather than a deliberately half-written system.

## 60.2 Rollback

Update state must identify:

- previous known-good state;
- staged state;
- active state;
- failed activation state.

---

# 61. BACKUP AND RESTORE

SB must eventually distinguish:

- system rollback;
- package-state rollback;
- configuration backup;
- user-data backup.

A system rollback must not silently overwrite user files.

---

# 62. CLEANUP / “UNNECESSARY DATA” POLICY

SB should actively minimize useless accumulated data.

Safe cleanup candidates may include:

- expired temporary files;
- unused package caches;
- stale downloaded artifacts;
- abandoned update staging;
- orphaned generated metadata.

Before deletion, classify the path.

Never use a broad “delete unknown files” rule as a cleanup mechanism.

## 62.1 Cleanup modes

### Automatic

Only low-risk classes such as expired TEMP/CACHE data.

### Suggested

Show the user what can be reclaimed.

### Manual

Allow advanced users to inspect categories directly.

---

# PART XII — APPLICATION MODEL

# 63. APPLICATION LIFECYCLE

```text
DISCOVERED
 ↓
INSTALLED
 ↓
LAUNCHING
 ↓
RUNNING
 ↓
SUSPENDED/BLOCKED if applicable
 ↓
EXITING
 ↓
EXITED
```

An application crash should normally not crash unrelated applications or the whole desktop.

---

# 64. APPLICATION IDENTIFICATION

An installed application should have a stable identity independent of its display name.

Potential metadata:

- app ID;
- version;
- executable entry;
- icon;
- permissions;
- supported files/protocols;
- language resources;
- package origin.

---

# PART XIII — MINECRAFT AND PERFORMANCE SPECIALIZATION

# 65. MINECRAFT ROLE

SB remains a general desktop OS, but Minecraft is an important first-class workload.

The kernel should remain generic.

Minecraft-specific behavior should live in higher-level policy/runtime components where possible.

Architecture:

```text
Generic Kernel
      ↓
Generic Scheduler / Memory / Storage / Network
      ↓
SB Runtime / optional optimization policy
      ↓
JVM / Minecraft tooling
      ↓
Minecraft / Minecraft Server
```

## 65.1 JVM integration

The OS should provide the hooks needed for an efficient JVM workload without baking one specific JVM implementation into the kernel.

## 65.2 Minecraft optimization principles

Potential optimization targets:

- scheduler policy for server workloads;
- filesystem/cache behavior;
- network latency;
- JVM memory placement;
- storage I/O;
- process priorities;
- performance monitoring.

Every optimization must be measured against generic workload regressions.

## 65.3 Minecraft Server Manager

Eventually provide a user-level manager capable of:

- creating server instances;
- selecting runtime versions;
- configuring memory;
- starting/stopping/restarting;
- viewing logs;
- backup/restore;
- network configuration;
- performance information.

It must use standard SB services rather than a privileged kernel shortcut.

## 65.4 Minecraft Instance Manager

Eventually manage client instances and optional runtime/tooling components while preserving the user's control over files and versions.

---

# 66. PERFORMANCE FRAMEWORK

Performance work must be measurable.

Track at minimum:

- boot time;
- time to desktop;
- idle RAM;
- idle CPU;
- background wakeups;
- GUI input latency;
- GUI frame latency;
- app startup latency;
- storage throughput/latency;
- network latency/throughput where applicable;
- package install time;
- update cost.

## 66.1 Performance policy

Do not “optimize” by:

- deleting safety checks;
- weakening tests;
- hiding errors;
- assuming hardware features;
- introducing unbounded complexity.

---

# PART XIV — USERS, ACCOUNTS AND PRIVILEGES

# 67. ACCOUNT SYSTEM

Eventually support:

- user identity;
- display name;
- account state;
- authentication method abstraction;
- home directory;
- groups/roles;
- permissions.

## 67.1 First user

Installer/first-run must establish the initial user according to an explicit account-creation policy.

Credentials must never be stored in plain text.

## 67.2 Administrative access

Administrative actions must be explicit.

The GUI should explain when an operation requires elevated privileges.

---

# 68. PERMISSIONS

Permission systems should distinguish:

- owner;
- group;
- other/public;
- privileged capability where needed.

Device access should use controlled permissions rather than giving every desktop application unrestricted hardware access.

---

# PART XV — INSTALLER AND LIVE SYSTEM

# 69. INSTALLER

The installer must eventually provide:

- target disk selection;
- installation mode;
- partitioning guidance;
- bootloader installation;
- base-package selection;
- account creation;
- language/keyboard setup;
- network setup where desired;
- progress and logging;
- validation before destructive actions;
- recovery on failure.

## 69.1 Destructive action safety

Before formatting or overwriting a disk:

- identify the target explicitly;
- show the intended operation;
- require clear confirmation;
- never infer consent from navigation.

## 69.2 Live environment

A future live environment should be able to:

- test hardware;
- run diagnostics;
- access files;
- repair installation;
- install SB.

The live environment should reuse system components where practical rather than becoming a completely separate OS.

---

# PART XVI — HARDWARE COMPATIBILITY STRATEGY

# 70. COMPATIBILITY MODEL

The target is not “support every device immediately.”

The target is:

**one clean architecture capable of supporting a very broad set of hardware over time.**

## 70.1 Initial baseline

- x86_64;
- QEMU;
- basic PCI;
- generic display;
- basic keyboard/mouse;
- basic storage path.

## 70.2 Compatibility classes

Eventually evaluate:

- generic desktop;
- laptop;
- gaming PC;
- workstation;
- high-end GPU system;
- professional GPU system;
- server-class machine;
- data-center-oriented machine.

## 70.3 Release artifact philosophy

Do not create a separate ISO for each GPU model.

Separate release artifacts only when:

- firmware requirements differ;
- kernel/driver composition differs materially;
- hardware compatibility testing justifies it;
- maintenance cost is acceptable.

Potential future channels:

```text
SB Desktop Generic
SB Desktop Gaming
SB Desktop Workstation
SB Desktop Hardware-specific support packs
```

---

# 71. SERVER / DATA-CENTER FUTURE COMPATIBILITY

The current product remains SB Desktop.

However, kernel and service boundaries should not make headless operation impossible later.

Future work may support:

- headless boot;
- serial console;
- remote administration services;
- server-oriented package profiles;
- high-core-count scheduling;
- large-memory systems;
- enterprise storage;
- high-performance networking.

This is compatibility planning, not a second current product scope.

---

# PART XVII — CI, TESTING AND RELEASE ENGINEERING

# 72. BUILD SYSTEM

The build must be reproducible enough that developers and automation can reproduce the same class of artifact.

Required build stages:

1. dependency validation;
2. compile;
3. assemble;
4. link;
5. image/ISO creation;
6. static validation;
7. boot validation;
8. artifact upload.

## 72.1 Architecture-specific assembly

The project has previously encountered unsupported relocation types caused by an architecture mismatch between boot assembly and linking.

Therefore:

- boot assembly mode must be explicit;
- relocation sizes must match symbol/address usage;
- mixed 32-bit/64-bit code must have explicit boundaries;
- CI must validate boot images after every relevant change.

---

# 73. TEST LEVELS

## Level 1 — host unit tests

Pure logic that can run outside the kernel.

## Level 2 — kernel self-tests

Small deterministic tests for PMM/VMM/heap/scheduler/etc.

## Level 3 — QEMU integration

Boot and interaction in an emulated machine.

## Level 4 — automated desktop tests

First-boot, settings, package and GUI flows where testable.

## Level 5 — real hardware

Device/display/input/network/GPU compatibility.

## Level 6 — release qualification

Full clean-install/upgrade/recovery scenarios.

---

# 74. REQUIRED QEMU BOOT TEST

The automated boot test must eventually verify at least:

```text
Kernel initialized
PCI discovery complete
PMM ready
VMM mapping self-test passed
Kernel heap self-test passed
GDT/TSS ready
Interrupt system ready
Scheduler ready
Process/Syscall ready
Userspace loader ready
Timer ready
Userspace init ready
Display service ready
GUI ready
First-boot path ready
```

As the implementation evolves, each previous line remains a regression test unless the architecture explicitly replaces it.

---

# 75. TEST FAILURE POLICY

When CI fails:

1. preserve logs;
2. identify the earliest failed invariant;
3. inspect the actual code;
4. reproduce in the smallest environment;
5. fix root cause;
6. add regression coverage;
7. rerun.

Do not “fix” CI by removing the assertion that detected the bug.

---

# 76. DEFINITION OF RELEASE READY

A release candidate is not ready because it builds.

It must satisfy:

- clean boot;
- clean install;
- first-run flow;
- language persistence;
- keyboard/input operation;
- storage operations;
- network operation;
- terminal operation;
- package install/remove/update;
- recovery behavior;
- error diagnostics;
- supported hardware claims;
- update/rollback tests;
- documentation;
- artifact integrity.

---

# PART XVIII — PUBLIC PROJECT / ECOSYSTEM

# 77. PROJECT WEBSITE

GitHub Pages is an appropriate initial public website mechanism because it avoids a paid hosting dependency.

The official website should eventually contain:

- project overview;
- download page;
- installation guide;
- hardware compatibility;
- documentation;
- changelog;
- source links;
- security information;
- support information;
- official community links.

The site must not claim support that the compatibility matrix cannot prove.

---

# 78. COMMUNITY CHANNELS

The project may maintain official:

- Discord;
- X/Twitter;
- GitHub Discussions/Issues;
- website announcements.

Community moderation and bot systems are separate from OS runtime engineering and must not become kernel dependencies.

---

# PART XIX — DEVELOPMENT REPOSITORY MODEL

# 79. EXPECTED REPOSITORY STRUCTURE

Conceptual target:

```text
SuiraBox-OS/
├─ boot/
├─ kernel/
│  ├─ arch/x86_64/
│  ├─ mm/
│  ├─ sched/
│  ├─ process/
│  ├─ syscall/
│  ├─ ipc/
│  ├─ drivers/
│  ├─ fs/
│  ├─ net/
│  ├─ time/
│  ├─ power/
│  ├─ log/
│  └─ panic/
├─ userspace/
│  ├─ init/
│  ├─ services/
│  ├─ gui/
│  └─ apps/
├─ docs/
├─ tests/
├─ site/
├─ .github/
├─ Makefile
├─ linker.ld
├─ README.md
└─ SB_OS_DESIGN.md
```

This is a target organization, not permission to create empty directories without implementation need.

---

# 80. DOCUMENTATION RELATIONSHIP

`SB_OS_DESIGN.md` is the master document.

Individual subsystem documents are subordinate references.

Example:

```text
SB_OS_DESIGN.md
   ↓
  docs/MEMORY.md
  docs/FILESYSTEM.md
  docs/SCHEDULER.md
  docs/DRIVERS.md
  docs/GUI_DESKTOP_PLAN.md
  docs/SB_STORE.md
  ...
```

If an individual document conflicts with this file, either:

- fix the individual document;
- or make a deliberate master-spec change.

Do not leave contradictory specifications indefinitely.

---

# PART XX — COMPLETE CONSTRUCTION ROADMAP

# 81. PHASE 0 — TOOLCHAIN

Deliverables:

- reproducible build command;
- compiler setup;
- assembler setup;
- linker setup;
- ISO creation;
- QEMU command;
- CI workflow.

Acceptance:

- clean checkout builds;
- generated ISO can be validated;
- QEMU reaches kernel entry.

# 82. PHASE 1 — EARLY BOOT

Deliverables:

- architecture entry;
- stack;
- boot protocol parsing;
- early console;
- linker symbols;
- reserved memory tracking.

Acceptance:

- boot logs deterministic;
- no invalid relocations;
- boot information survives until consumed.

# 83. PHASE 2 — MEMORY

Deliverables:

- PMM;
- VMM;
- heap;
- page-fault foundation.

Acceptance:

- allocation/free self-tests pass;
- mapping tests pass;
- reserved memory is never returned.

# 84. PHASE 3 — CPU/INTERRUPTS/TIME

Deliverables:

- GDT/TSS;
- IDT;
- exception handlers;
- PIC/APIC;
- timer;
- synchronization.

Acceptance:

- timer interrupts occur;
- exceptions produce controlled diagnostics;
- no illegal blocking in interrupt paths.

# 85. PHASE 4 — EXECUTION

Deliverables:

- scheduler;
- threads;
- processes;
- address spaces;
- IPC;
- syscalls;
- ELF loader;
- userspace init.

Acceptance:

- at least one real userspace process runs safely;
- syscall boundary validates pointers;
- process cleanup works.

# 86. PHASE 5 — STORAGE

Deliverables:

- block devices;
- partitioning;
- VFS;
- filesystem;
- configuration service.

Acceptance:

- persistent read/write works;
- configuration survives reboot;
- corrupted configuration follows recovery policy.

# 87. PHASE 6 — DEVICE INPUT/OUTPUT

Deliverables:

- PCI;
- storage drivers;
- keyboard;
- mouse;
- display;
- audio foundation;
- network interface foundation.

Acceptance:

- generic PC in supported test environment can operate the required devices.

# 88. PHASE 7 — GUI

Deliverables:

- font/text;
- surfaces;
- input events;
- compositor;
- window system;
- toolkit;
- desktop shell;
- file manager.

Acceptance:

- desktop usable with keyboard/mouse;
- applications can open/close;
- errors are GUI-visible where appropriate.

# 89. PHASE 8 — FIRST BOOT / LOCALIZATION

Deliverables:

- configuration persistence;
- language catalogs;
- language selector;
- keyboard layout selection;
- IME architecture;
- localized Settings.

Acceptance:

- clean install shows graphical language selector;
- selected language persists after reboot;
- keyboard layout matches user's selection.

# 90. PHASE 9 — NETWORK

Deliverables:

- sockets;
- DHCP;
- DNS;
- Ethernet;
- IPv4/IPv6;
- network manager.

Acceptance:

- user can configure and diagnose network from GUI and CLI.

# 91. PHASE 10 — PACKAGE ECOSYSTEM

Deliverables:

- package format;
- repository metadata;
- downloader;
- transaction engine;
- SB Store;
- cache management.

Acceptance:

- install/remove/update from GUI and CLI produce identical state.

# 92. PHASE 11 — SECURITY/RECOVERY

Deliverables:

- permission model;
- package signatures;
- update verification;
- crash dump;
- support report;
- recovery environment;
- rollback.

Acceptance:

- defined failure scenarios recover safely.

# 93. PHASE 12 — PERFORMANCE / COMPATIBILITY

Deliverables:

- performance suite;
- memory reduction;
- boot optimization;
- broader device support;
- GPU work.

Acceptance:

- performance claims backed by measurements;
- compatibility matrix updated with test evidence.

# 94. PHASE 13 — RELEASE

Deliverables:

- release candidate;
- documentation;
- installer;
- checksums/signing;
- website;
- release notes.

Acceptance:

- release test matrix passes.

---

# PART XXI — EXACT INTERACTION BETWEEN MAJOR SUBSYSTEMS

# 95. BOOT → MEMORY

Boot parser identifies protected ranges.

PMM cannot reclaim those ranges until the last dependent component declares them releasable.

# 96. MEMORY → SCHEDULER

Scheduler allocations are made through kernel memory interfaces.

A scheduler task may not own a page without PMM/VMM bookkeeping reflecting that ownership.

# 97. SCHEDULER → USERSpace

The scheduler provides CPU execution; it does not know GUI semantics.

# 98. USERSpace → STORAGE

Applications use VFS/syscalls.

They do not directly interpret raw block-device sectors.

# 99. STORAGE → CONFIG

Configuration data is persisted atomically through VFS/storage services.

# 100. CONFIG → FIRST BOOT

First-run checks configuration validity.

No valid configuration → first-run UI.

Valid configuration → skip first-run.

# 101. DISPLAY → GUI

Display service provides rendering targets.

GUI never assumes a specific GPU vendor.

# 102. INPUT → GUI

Raw device events are normalized by input services before toolkit consumption.

# 103. NETWORK → STORE

SB Store uses network services.

The Store does not own NIC configuration.

# 104. PACKAGE SERVICE → STORE + CLI

Store and CLI share the same transaction engine and package state.

# 105. ERROR SERVICE → EVERY SUBSYSTEM

Every layer can produce an error record appropriate to its level.

A lower-level subsystem must have a lower-level diagnostic path that remains available if the normal UI is unavailable.

---

# PART XXII — FAILURE SCENARIOS THAT MUST BE DESIGNED EXPLICITLY

# 106. PMM FAILURE

Possible causes:

- invalid map;
- reserved range collision;
- allocator corruption;
- out of memory;
- metadata overlap.

Required response:

- record the exact allocator state;
- stop unsafe allocation;
- avoid recursive logging;
- enter the smallest safe diagnostic/recovery state.

# 107. VMM FAILURE

- distinguish mapping failure from page-fault failure;
- capture virtual/physical addresses;
- preserve process identity;
- do not continue with corrupted page tables.

# 108. SCHEDULER FAILURE

- capture current CPU/thread;
- capture runqueue state where safe;
- do not recursively invoke scheduler recovery from the scheduler itself;
- choose panic if invariants are compromised.

# 109. STORAGE FAILURE

Prefer:

- retry;
- offline device;
- readonly fallback where safe;
- notification;
- recovery filesystem path.

Do not pretend writes succeeded.

# 110. NETWORK FAILURE

Network loss is normally recoverable.

Do not escalate network failure to BSOD/RSOD merely because an application depends on networking.

# 111. DISPLAY FAILURE

Try:

- restart display service;
- fallback framebuffer;
- recovery display.

Escalate to RSOD only if the display/recovery path itself cannot be trusted.

# 112. PACKAGE FAILURE

- abort transaction;
- restore previous package state;
- retain logs;
- show package/error ID;
- do not leave the package database claiming success when activation failed.

---

# PART XXIII — ENGINEERING RULES FOR THE AI/DEVELOPER

# 113. REQUIRED WORK LOOP

```text
Read SB_OS_DESIGN.md
 ↓
Inspect repository
 ↓
Inspect actual implementation
 ↓
Inspect latest relevant CI
 ↓
Identify earliest missing prerequisite
 ↓
Implement root-cause change
 ↓
Build
 ↓
Run runtime test
 ↓
Inspect logs
 ↓
Add regression coverage
 ↓
Update design if architecture changed
 ↓
Record actual status
 ↓
Proceed to next dependency
```

## 113.1 Forbidden shortcuts

Never:

- claim an untested feature works;
- delete a failing test to make CI pass;
- silently change requirements;
- introduce duplicate sources of truth;
- make a lower layer depend on GUI code;
- silently erase user data;
- call a mockup a functioning OS subsystem;
- add fake hardware support;
- create compatibility claims from detection alone.

## 113.2 Temporary code

Temporary diagnostics must be:

- identifiable;
- bounded;
- non-invasive;
- removed or deliberately retained after investigation.

---

# PART XXIV — PROJECT STATE AND RECOVERY

# 114. CURRENT REAL PROJECT STATE SNAPSHOT

At the time this specification was prepared, the repository already contains significant early-kernel infrastructure and individual subsystem design documents.

The known runtime bottleneck is the PMM initialization path in QEMU.

Current demonstrated progression is approximately:

```text
Build toolchain           verified
ISO generation            verified
Multiboot validation      verified
Kernel entry              verified
PCI scan                  verified
Storage bootstrap         reached
PMM initialization        current blocker
VMM                       not yet reached in successful smoke path
Heap                      not yet reached in successful smoke path
Scheduler                 not yet reached in successful smoke path
Userspace                 not yet reached in successful smoke path
GUI                       not yet reached in successful smoke path
```

This status is not permanent and must be updated as CI results change.

## 114.1 Current PMM investigation principle

Do not assume that a 20-second timeout means the PMM algorithm is computationally expensive.

The investigation must distinguish:

- function-entry failure;
- static-storage access fault;
- bitmap write failure;
- range-processing error;
- page-map/access issue;
- earlier memory corruption.

Diagnostic output must itself avoid depending on the subsystem under investigation.

---

# PART XXV — FINAL PRODUCT ACCEPTANCE

# 115. SB DESKTOP V1 DEFINITION OF DONE

SB Desktop is considered release-ready only when a supported machine can:

1. boot into SB through the supported boot path;
2. initialize hardware safely;
3. initialize memory safely;
4. run userspace safely;
5. initialize storage;
6. initialize input;
7. initialize display;
8. start the desktop;
9. display first-run GUI setup on a clean configuration;
10. allow language selection with a graphical select control;
11. allow keyboard-layout configuration;
12. persist those settings;
13. present a usable desktop;
14. run applications as isolated userspace processes;
15. use files safely;
16. configure a network;
17. use a functional terminal;
18. install/remove/update optional software;
19. handle package transactions safely;
20. expose detailed settings;
21. display normal errors without catastrophic-style screens;
22. generate useful diagnostics for serious failures;
23. recover supported service/application/package failures;
24. survive reboot with valid persistent configuration;
25. provide truthful hardware-support claims;
26. pass defined CI/runtime tests;
27. pass release qualification.

---

# PART XXVI — DECISIONS THAT MUST REMAIN EXPLICITLY UNDECIDED UNTIL ENGINEERING VALIDATION

The following must not be invented prematurely:

- final filesystem implementation;
- final package container format;
- final package repository protocol;
- final GUI protocol/wire format;
- final sandbox implementation;
- final authentication provider;
- final JVM distribution strategy;
- final GPU userspace API choices;
- final long-term update image layout;
- final multi-disk/storage redundancy strategy.

When one of these is decided, update this master document with:

1. decision;
2. alternatives considered;
3. reason;
4. compatibility impact;
5. migration requirement;
6. test requirement.

---

# PART XXVII — ARCHITECTURAL DECISION RECORD FORMAT

Every major irreversible decision should eventually be recorded in this file in this form:

```text
Decision ID:
Date:
Decision:
Problem:
Options considered:
Chosen option:
Reason:
Security impact:
Performance impact:
Compatibility impact:
Migration requirement:
Tests required:
Status:
```

This prevents the project from losing the reasons behind major architecture choices.

---

# PART XXVIII — MASTER PRINCIPLE

SB is not supposed to become a giant collection of features that happen to boot.

It is supposed to become a coherent operating system in which:

```text
Minimal base
      +
Optional components
      +
Detailed but understandable control
      +
Real CLI
      +
Strong hardware abstraction
      +
Fast installation
      +
Clear diagnostics
      +
Recoverable failures
      +
Measured performance
      +
Broad future compatibility
      =
SuiraBox OS
```

The desktop should feel simple to a normal user while remaining technically deep underneath.

The kernel should remain small enough to reason about while the userspace ecosystem grows.

Optional functionality should be downloadable rather than permanently embedded into the minimum installation whenever that materially improves the system.

The user should be able to control the machine without being forced to understand kernel internals.

The developer should be able to understand the kernel without reverse-engineering undocumented behavior.

The support process should be able to determine what failed without guessing.

The project should remain recoverable even if an individual conversation, developer, machine or AI session disappears.

**This file is the master memory of that design.**

---

# APPENDIX A — MASTER REQUIREMENT INDEX

The following classes should eventually be tracked individually in project tooling:

```text
BOOT-*       boot and firmware
ARCH-*       architecture/CPU
MEM-*        PMM/VMM/heap
INT-*        interrupts/exceptions/timers
SMP-*        multicore
SYNC-*       synchronization
SCHED-*      scheduler
PROC-*       process/thread
IPC-*        IPC
SYS-*        syscalls/ABI
EXEC-*       executable loading
STORE-*      storage/VFS/filesystem
CFG-*        configuration
TIME-*       time/locale
PCI-*        PCI
DRV-*        drivers
USB-*        USB/hotplug
INPUT-*      input
DISP-*       display
GPU-*        graphics
AUDIO-*      audio
POWER-*      power
GUI-*        GUI toolkit
DESK-*       desktop shell
SET-*        settings
I18N-*       localization
IME-*        input methods
A11Y-*       accessibility
NET-*        networking
TERM-*       terminal
PKG-*        package manager
STOREUI-*    SB Store
SEC-*        security
ERR-*        normal errors
PANIC-*      BSOD/RSOD/panic
DIAG-*       diagnostics
REC-*        recovery
UPD-*        updates
DATA-*       data lifecycle/cleanup
APP-*        applications
MC-*         Minecraft/JVM/server tooling
PERF-*       performance
INST-*       installer/live environment
CI-*         CI/build
REL-*        releases
WEB-*        public website/ecosystem
DOC-*        documentation
AI-*         AI handoff/protocol
```

---

# APPENDIX B — FINAL TEST MATRIX CATEGORIES

Before a major release, evaluate at least:

```text
BOOT
REBOOT
SHUTDOWN
CLEAN INSTALL
UPGRADE
ROLLBACK
RECOVERY
LANGUAGE
KEYBOARD LAYOUT
IME
UNICODE
DISPLAY
MULTI-DISPLAY where supported
MOUSE
KEYBOARD
AUDIO
NETWORK
DNS
DHCP
TERMINAL
FILESYSTEM
PACKAGE INSTALL
PACKAGE REMOVE
PACKAGE UPDATE
PACKAGE CONCURRENCY
ERROR UX
CRASH REPORT
SUPPORT REPORT
PRIVILEGE BOUNDARY
USER ISOLATION
GPU FALLBACK
GPU ACCELERATION where claimed
POWER MANAGEMENT
PERFORMANCE
CACHE CLEANUP
DATA SAFETY
QEMU
REAL HARDWARE
```

---

# APPENDIX C — OFFICIAL EXTERNAL SPECIFICATION SOURCES TO CONSULT

The implementation must rely on current authoritative specifications rather than memory or informal assumptions.

- Intel® 64 and IA-32 Architectures Software Developer Manuals for processor architecture, memory management, protection, interrupts, multiprocessing and system programming. citeturn511015search0turn511015search9
- UEFI Forum specifications for UEFI firmware interfaces. The current published UEFI specification is version 2.11. citeturn511015search3turn511015search5
- PCI-SIG specifications for PCI Express and related platform/device behavior. citeturn511015search4turn511015search11
- USB-IF documentation for USB/USB Type-C/UCSI interfaces and compliance material. citeturn342371search3turn342371search4
- NVM Express specifications for NVMe storage. The current published NVMe set includes NVMe Base 2.4 and associated command/transport specifications as of August 2026. citeturn342371search0turn342371search1
- Khronos Vulkan specification if/when Vulkan is adopted as a userspace graphics API. citeturn342371search60

These references are normative external inputs. SB-specific behavior must still be documented in this file rather than left implicit.

---

# APPENDIX D — THE SINGLE MOST IMPORTANT RULE

When a future developer or AI opens this repository and asks:

> “What is SB supposed to be?”

the first answer must be:

**Read `SB_OS_DESIGN.md`.**

When they ask:

> “What is actually implemented?”

the answer must be obtained from:

**the code + tests + CI + runtime evidence.**

When they ask:

> “What should I implement next?”

the answer must be:

**the earliest incomplete prerequisite in the dependency order defined by this document.**

That is the mechanism that keeps SB coherent as the project grows across developers, AI agents, machines, releases and years of development.
