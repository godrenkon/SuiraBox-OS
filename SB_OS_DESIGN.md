# SuiraBox OS — Master Product, Architecture, Construction, UX, Compatibility and Recovery Specification

> **This is the canonical master specification for SuiraBox OS (SB Desktop).**
>
> This file is simultaneously the product definition, complete architectural blueprint, implementation plan, operational reference, recovery manual, and AI handoff document for the SB Desktop project.
>
> The intention is not to describe only the kernel or only the GUI. The intention is to record **what SB is, what the finished OS contains, how the pieces interact, what the user sees, how the system behaves, how it fails, how it recovers, how it is installed and updated, how optional functionality is distributed, how hardware is supported, how Minecraft-focused optimization fits into the architecture, and how the project is verified and released.**
>
> **Current active product:** SB Desktop.
>
> **Independent CUI-only operating-system edition:** out of current scope. SB Desktop itself must still contain a real terminal and CLI because CLI operation is a first-class capability.
>
> **Status rule:** A description in this document is a requirement or intended design. It is not proof of implementation. Implementation state is determined by repository contents, builds, automated tests, QEMU runs, hardware tests and release verification.

---

# PART I — PRODUCT DEFINITION

## 1. Identity

### 1.1 Names

- Project: **SuiraBox OS**
- Short name: **SB**
- Current desktop product: **SB Desktop**
- Project/community identity: **Suiram**
- Primary repository: `godrenkon/SuiraBox-OS`
- Primary development account/owner identity currently used in repository: `godrenkon`

### 1.2 What SB is

SB is an open-source, modular, lightweight, configurable desktop operating system intended to provide:

- a normal graphical desktop experience;
- strong command-line capability;
- efficient use of CPU, RAM and storage;
- fine-grained but understandable configuration;
- optional software/components installed on demand;
- fast and verified package downloads;
- strong diagnostics and understandable failures;
- recoverable failures where technically safe;
- broad hardware compatibility through abstractions and incremental driver support;
- an architecture suitable for Minecraft-focused performance optimization without turning the entire kernel into Minecraft-specific code;
- a path from a minimal generic system to gaming, workstation and other optimized deployments later.

### 1.3 What SB is not

SB is not intended to:

- force every available feature into the base installation;
- require terminal commands for ordinary first setup;
- hide technical failures behind meaningless generic messages;
- silently delete user files to save space;
- claim hardware compatibility that has not actually been tested;
- turn every optional component into a permanently running service;
- duplicate subsystem logic separately for GUI and CLI when one shared backend is practical.

### 1.4 Primary design objective

The primary objective is **maximum useful capability per unit of system complexity**.

The OS should be as small and efficient as reasonably possible without sacrificing:

- correctness;
- security;
- recoverability;
- compatibility;
- maintainability;
- user control;
- diagnostics.

A small binary that is unreliable is not considered successful lightweight design.

---

## 2. Core philosophy

### 2.1 Minimal base

The installed base system contains only functionality necessary for a secure, usable desktop and its essential runtime dependencies.

Optional applications, language packs, fonts, development tools, advanced drivers and similar non-essential capabilities should be independently distributable where doing so has a real benefit.

### 2.2 Choice without confusion

Users should be able to choose detailed behavior without being forced to understand kernel internals.

Therefore every advanced setting should have:

- a human-readable name;
- a concise description;
- a safe default;
- a validation rule;
- a place to find advanced technical details when necessary;
- an indication of whether changing it requires restart, logout or service restart.

### 2.3 GUI first, CLI fully functional

The graphical desktop is the ordinary user interface.

The terminal is not a replacement for the GUI. It is an additional interface for advanced users, development, recovery and automation.

GUI and CLI should use common underlying services wherever feasible so that:

- a function available in the GUI also has a consistent CLI/API path where appropriate;
- a CLI operation does not require a hidden GUI service;
- documentation can describe one subsystem instead of two unrelated implementations.

### 2.4 Optional components

The base OS should not carry unnecessary long-lived runtime cost simply because an application or component might eventually be needed.

The package architecture must distinguish at least:

- required base component;
- optional system component;
- application;
- development component;
- language/localization component;
- driver/firmware component where legally distributable;
- cache/data generated at runtime.

### 2.5 User-centered recovery

A failure should be handled at the lowest safe severity.

Preferred hierarchy:

```text
Application failure
    ↓
Restart application
    ↓
Restart affected service
    ↓
Restart affected userspace subsystem
    ↓
Recover kernel subsystem if technically safe
    ↓
Recovery boot
    ↓
System stop / kernel panic
```

The OS must not turn an ordinary application error into a BSOD or RSOD merely because a visual error screen looks impressive.

---

## 3. Finished SB Desktop — complete product surface

The finished SB Desktop is composed of the following major surfaces.

### 3.1 Boot and installation

- Firmware compatibility layer.
- Bootloader.
- Boot diagnostics.
- Installer.
- Live/recovery environment as development permits.
- Clean-install flow.
- Upgrade path.
- Recovery path.

### 3.2 Kernel

- Architecture layer.
- CPU initialization.
- Memory management.
- Interrupts/exceptions.
- Timers.
- Scheduler.
- Process/thread model.
- IPC.
- Syscall ABI.
- Security primitives.
- Kernel logging.
- Kernel panic infrastructure.

### 3.3 Device and hardware services

- PCI.
- ACPI.
- CPU topology.
- RAM discovery.
- Storage controllers/devices.
- USB.
- Input devices.
- Display/framebuffer.
- GPU acceleration.
- Network devices.
- Audio.
- Power management.
- Hotplug where supported.

### 3.4 Userspace platform

- Init/service manager.
- Configuration service.
- Account service.
- Display service.
- Input service.
- Network manager.
- Package manager.
- Update manager.
- Logging/diagnostics service.
- Notification service.
- Time/locale service.

### 3.5 Desktop

- Compositor.
- Window system.
- UI toolkit.
- Desktop shell.
- Application launcher.
- Task/window switcher.
- Notifications.
- System status area.
- File manager.
- Settings.
- Terminal.
- System dialogs.
- Power controls.

### 3.6 Distribution platform

- Package format.
- Repository metadata.
- Package verification.
- Download manager.
- Transaction engine.
- Cache.
- SB Store.
- Update/rollback mechanisms.

### 3.7 Diagnostics and recovery

- Normal errors.
- Structured logs.
- Error IDs.
- Technical details.
- Support report.
- Crash data.
- BSOD.
- RSOD.
- Recovery mode.
- Rollback.

### 3.8 Minecraft-focused ecosystem

SB remains a general desktop OS, but its architecture must permit a dedicated upper-layer ecosystem for Minecraft-related use:

- Minecraft Java Edition runtime integration;
- JVM/runtime management;
- Minecraft instance management;
- Minecraft Server management;
- performance policies tuned for Minecraft workloads;
- benchmark tooling;
- storage/cache policies useful for game assets;
- network/CPU scheduling policies that improve applicable workloads.

Minecraft optimization belongs primarily in **policy, runtime, service and application layers**, not as arbitrary Minecraft-specific branches throughout the generic kernel.

---

# PART II — USER EXPERIENCE

## 4. Boot-to-desktop lifecycle

The conceptual finished flow is:

```text
Power On
  ↓
Firmware initialization
  ↓
Bootloader
  ↓
Kernel entry
  ↓
Early CPU state
  ↓
Early memory/boot information protection
  ↓
Kernel memory infrastructure
  ↓
CPU/interrupt/timer infrastructure
  ↓
Scheduler + process infrastructure
  ↓
Userspace init
  ↓
Storage + configuration
  ↓
Display + input
  ↓
Compositor
  ↓
Desktop Shell
  ↓
First-run check
  ↓
Language selector when required
  ↓
Normal Desktop
```

The system must not require a command-line interaction for a normal first graphical setup.

---

## 5. First graphical startup

### 5.1 Trigger condition

Show first-run UI when the OS cannot find a valid persistent user/system setup record containing the minimum required locale/language initialization state.

### 5.2 Visual form

The first-run language selector is a small graphical popup over the desktop.

Conceptual UI:

```text
Welcome to SuiraBox

Language
[ English ▼ ]

[ Continue ]
```

The actual visual theme may evolve, but the interaction model is fixed: **a graphical select/drop-down control plus a clear continue action.**

### 5.3 Initial language choices

The initial supported UI languages are:

- 日本語
- English
- 中文
- Español

Do not require the user to enter a locale code.

### 5.4 First-run sequence

1. Display the popup.
2. Populate language choices from the built-in language catalog.
3. Determine a safe initial selection.
4. User chooses language.
5. Validate the selection.
6. Determine initial locale defaults.
7. Determine keyboard-layout default separately from UI language.
8. Allow keyboard layout correction before credential entry or other input-heavy setup.
9. Persist configuration atomically.
10. Activate the selected language.
11. Refresh or restart affected UI components as required.
12. Mark first-run initialization complete.
13. Continue to the normal desktop.

### 5.5 Language versus keyboard layout

UI language and physical/input keyboard layout are separate concepts.

Example:

```text
UI language: Japanese
Keyboard layout: Japanese JIS
```

or:

```text
UI language: English
Keyboard layout: US
```

The system must not assume that language uniquely determines keyboard layout.

The user must be able to change the keyboard layout later from Settings.

### 5.6 Recovery from broken first-run configuration

If the configuration is missing, invalid, partially written or incompatible with the current schema:

- validate before use;
- attempt migration if a supported prior version exists;
- otherwise fall back to safe defaults;
- return to setup only when required;
- never enter an infinite setup/reboot loop.

---

## 6. Desktop shell

The desktop shell is the normal container for user interaction.

It must expose, directly or through clearly discoverable UI:

- launcher;
- open applications/windows;
- current workspace/window state;
- network status;
- audio state;
- time/date;
- notifications;
- power controls;
- Settings;
- File Manager;
- Terminal.

The shell must remain lightweight and should lazy-load optional UI functionality.

---

## 7. Settings

Settings is a real application backed by the configuration service.

Categories should include at least:

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
- Recovery
- Performance
- Developer

### 7.1 Configuration object model

Every persistent setting should have:

- stable key/ID;
- value type;
- default value;
- valid range/enum/schema;
- owner subsystem;
- persistence location;
- migration strategy;
- effect classification;
- required privilege level.

### 7.2 Configuration transaction rules

A configuration update should not leave a malformed partially written file.

Preferred sequence:

```text
Read current state
  ↓
Validate proposed state
  ↓
Create staged representation
  ↓
Durably write/stage
  ↓
Atomically activate
  ↓
Notify owner subsystem
```

---

## 8. Localization

### 8.1 Architecture

User-visible strings use stable message IDs rather than hard-coded duplicated strings.

Conceptual examples:

```text
ui.welcome.title
ui.language.label
ui.continue
error.network.timeout
error.storage.read_failed
panic.page_fault
settings.display.refresh_rate
```

### 8.2 Requirements

- Unicode support.
- UTF-aware text handling.
- Font fallback.
- Missing-translation fallback.
- Safe string expansion.
- No layout corruption from long translations.
- Translation packages separable from the minimal base when practical.

### 8.3 Input methods

UI language support and text input support are separate layers.

The architecture must eventually support appropriate input methods for supported languages, including Japanese and Chinese input, without hard-coding language logic into GUI widgets.

---

## 9. Accessibility

The finished desktop should expose a real accessibility layer rather than visually styling controls only.

Target capabilities include:

- keyboard navigation;
- focus indicators;
- scalable UI text;
- high-contrast configuration;
- reduced-motion option;
- accessible labels/roles;
- notification duration control;
- screen-reader integration path;
- color-independent status indicators.

Accessibility is part of the GUI platform, not a collection of application-specific hacks.

---

# PART III — BOOT, CPU AND MEMORY

## 10. Firmware boundary

SB must be prepared for both legacy BIOS-style boot environments and modern UEFI-based systems through a defined bootloader boundary.

UEFI and ACPI are external standards and must be implemented according to their applicable specifications rather than approximated from assumptions. The UEFI Forum currently publishes UEFI 2.11 and ACPI 6.6 as current specification versions. 

The boot architecture must isolate firmware-specific handling from the common kernel model.

### 10.1 Boot information normalization

Firmware/bootloader-specific data is converted into SB internal structures such as:

- physical memory regions;
- CPU topology;
- ACPI table locations;
- framebuffer information;
- boot modules;
- firmware/reserved regions;
- device discovery hints.

Higher layers must not need to know whether a specific record originated from Multiboot2, UEFI or another future boot path.

---

## 11. Multiboot2 handling

Multiboot2 provides a tagged boot-information structure. The boot information is not owned by the PMM merely because it lies in physical memory.

The boot path must:

1. validate the boot-information pointer;
2. validate total size and tag boundaries;
3. validate individual tag size/alignment before reading;
4. identify all boot information that will remain needed;
5. reserve the physical memory occupied by those structures while they are referenced;
6. copy data into SB-owned structures when appropriate;
7. release the original boot-information memory only after its final consumer is finished.

This is required because the Multiboot2 specification places responsibility on the OS to avoid overwriting boot information while it is still in use. citeturn441644search30

### 11.1 Memory-map precedence

When multiple sources provide memory information, the source's semantic authority must be documented. The system must not blindly merge incompatible ranges.

### 11.2 Malformed input

Malformed or contradictory boot data must produce a controlled boot diagnostic rather than unsafe memory reuse.

---

## 12. CPU initialization

The architecture abstraction must isolate x86_64-specific operations.

### 12.1 C

C is the primary kernel implementation language.

### 12.2 x86_64 Assembly

Assembly is reserved for operations where it is the appropriate implementation boundary:

- boot entry;
- low-level CPU state changes;
- interrupt entry/exit stubs;
- context-switch primitives;
- syscall entry/exit;
- selected CPU instructions.

### 12.3 Linker script

The linker script owns kernel image placement and exports symbols for:

- kernel start/end;
- sections;
- early stack;
- boot metadata as required;
- alignment boundaries.

### 12.4 Architecture invariants

The implementation must not mix 32-bit and 64-bit relocation assumptions.

All linker symbols and assembly references must have explicitly reviewed operand width and relocation semantics.

---

## 13. GDT, TSS and IDT

### GDT

Provide the segmentation descriptors required by the chosen x86_64 execution model, including kernel and user execution/data contexts where userspace is supported.

### TSS

Provide task-state information required for privilege transitions and stack switching.

### IDT

Provide interrupt and exception vectors with correct gate types, privilege levels and entry stubs.

Every vector must have a defined handler behavior.

---

## 14. Exception model

Each exception handler must determine:

- exception/vector;
- error code where applicable;
- faulting instruction pointer;
- relevant register context;
- active process/thread;
- current privilege level;
- recoverability;
- whether the fault originated in kernel or userspace.

Userspace faults should generally terminate or signal the affected task rather than stopping the entire machine.

Kernel faults enter the panic path when safe continuation cannot be established.

---

## 15. Physical Memory Manager (PMM)

### 15.1 Purpose

PMM owns allocation and reservation of physical pages.

### 15.2 Page granularity

The initial x86_64 implementation uses the architecture-defined standard page size as its base allocation unit, normally 4 KiB.

The page size is centralized, not duplicated across subsystems.

### 15.3 Region classes

Physical memory must be represented using explicit state categories such as:

```text
FREE
RESERVED
KERNEL
BOOT_DATA
BOOT_MODULE
PAGE_TABLE
PMM_METADATA
FIRMWARE
ACPI
DEVICE/MMIO
ALLOCATED
BAD/UNUSABLE
```

Exact internal enum names may vary; the semantic distinction must remain.

### 15.4 Initialization lifecycle

The PMM must not release memory that is still required by:

- kernel image;
- current stack;
- page tables;
- boot information;
- boot modules;
- PMM metadata;
- ACPI/firmware structures still in use;
- device/MMIO reservations.

### 15.5 Bootstrap PMM

A bounded bootstrap allocator may be used before the complete platform memory map is integrated.

The bootstrap allocator must not be treated as proof of final memory topology.

### 15.6 API contract

Conceptual operations:

```text
pmm_init(...)
pmm_add_usable_range(start, end)
pmm_reserve_range(start, end)
pmm_alloc_page()
pmm_free_page(page)
pmm_total_pages()
pmm_free_pages()
```

Each operation requires:

- alignment validation;
- range validation;
- ownership validation;
- defined failure behavior;
- concurrency policy once SMP is active.

### 15.7 PMM invariants

- never return a reserved page;
- never return an out-of-range page;
- never accept an invalid free silently where detection is possible;
- prevent double-free corruption;
- preserve allocation state across subsystem startup;
- avoid unbounded initialization loops;
- never depend on the GUI/logger to function.

---

## 16. Virtual Memory Manager (VMM)

VMM owns virtual address spaces and page mappings.

### 16.1 Responsibilities

- page-table creation;
- mapping;
- unmapping;
- permissions;
- address-space creation/destruction;
- kernel/user separation;
- fault handling;
- TLB management;
- page-table memory ownership through PMM.

### 16.2 Protection model

At minimum support conceptual attributes:

```text
PRESENT
WRITABLE
USER
EXECUTABLE/NX
CACHE POLICY
```

The final bit-level implementation must match x86_64 paging semantics.

### 16.3 Invariants

- user mappings cannot arbitrarily expose kernel-only pages;
- unmap must return ownership information appropriately;
- physical page ownership must be unambiguous;
- page-alignment requirements enforced;
- malformed user pointers never bypass validation.

---

## 17. Kernel heap

The kernel heap provides sub-page dynamic allocation built on lower-level memory primitives.

Requirements:

- alignment guarantees;
- overflow detection;
- invalid-free detection where practical;
- deterministic out-of-memory behavior;
- no hidden dependency on GUI/logging;
- fragmentation testing;
- optional debug allocator instrumentation.

The kernel heap must not replace PMM/VMM; those lower-level mechanisms remain available.

---

# PART IV — EXECUTION, INTERRUPTS, SCHEDULING AND IPC

## 18. Interrupt controller and timer architecture

### 18.1 Early boot timer

A simple legacy timer path such as PIT may be used for early bring-up.

### 18.2 Mature timer path

When APIC infrastructure is available, the scheduler should migrate to a per-CPU capable timer mechanism such as the Local APIC timer where supported.

The timer abstraction must hide this transition from scheduler policy.

Conceptual phases:

```text
Early timer
  ↓
Interrupt infrastructure verified
  ↓
APIC/CPU topology initialized
  ↓
Per-CPU timer path enabled
  ↓
Scheduler uses stable timer abstraction
```

### 18.3 Timebase rules

Do not let scheduling correctness depend on a fragile wall-clock source.

Separate:

- monotonic time;
- wall-clock time;
- RTC/firmware time;
- timezone/locale presentation.

---

## 19. ACPI

ACPI provides platform configuration and power-management information.

Initial responsibilities include:

- discovering relevant tables;
- validating table signatures/checksums as required;
- locating processor and interrupt topology information;
- power-state information;
- device/resource information where required.

ACPI parsing must be separated from the power-management policy itself.

ACPI data structures remain protected while referenced.

The project must track applicable ACPI revisions rather than assuming one fixed version forever. The UEFI Forum lists ACPI 6.6 as the current published specification at the time of this document. citeturn441644search0

---

## 20. SMP / multicore

The final architecture must support multiple CPUs without changing the conceptual scheduler/process API.

Per-CPU state should include, as required:

- CPU identifier;
- current thread;
- scheduler state;
- interrupt/preemption state;
- kernel stack/temporary state;
- per-CPU timer state;
- performance counters where later supported.

Required concerns:

- startup of secondary CPUs;
- synchronization;
- interrupt routing;
- memory barriers;
- cache coherence assumptions;
- per-CPU scheduler queues or equivalent;
- safe process/address-space access.

Single-core operation remains a supported development mode.

---

## 21. Synchronization and concurrency

Provide explicit primitives rather than ad-hoc interrupt disabling everywhere.

Potential primitives:

- spinlock;
- mutex/sleeping lock;
- semaphore;
- condition/event;
- reader/writer synchronization;
- atomic operations;
- wait queues.

### 21.1 Lock ordering

Subsystems that may acquire multiple locks must document lock order to prevent deadlock.

### 21.2 Interrupt context

A function callable from interrupt context must be explicitly classified as such.

Operations forbidden or restricted in hard interrupt context must include at least:

- potentially blocking sleeps;
- unbounded waits;
- ordinary heap allocation unless allocator explicitly supports it;
- operations that take locks known to be held by interrupted code.

---

## 22. Kernel logging

Kernel logging is layered by execution context.

### 22.1 Early logger

Must require as little infrastructure as possible.

### 22.2 Interrupt-safe logging

In contexts where locks or blocking are unsafe, logging must use a minimal non-blocking or bounded path, typically a fixed-size ring buffer or direct emergency output.

### 22.3 Normal logger

Userspace-visible logging can later consume structured kernel events through a dedicated service.

### 22.4 No logging recursion

The logger itself must not silently call a subsystem whose failure it is attempting to report.

For example:

```text
PMM failure
  ↓
logging
  ↓
heap allocation
  ↓
PMM
```

is an invalid hidden dependency.

---

## 23. Scheduler

### 23.1 Responsibilities

- select runnable threads;
- account CPU time;
- block/sleep/wake threads;
- perform context switches;
- honor priority policy;
- handle preemption where enabled;
- cooperate with CPU topology.

### 23.2 Policy abstraction

Generic scheduler mechanisms must remain independent of Minecraft-specific optimization policies.

Minecraft/game tuning should operate through explicit scheduling policy interfaces or runtime hints rather than hard-coded assumptions.

### 23.3 Scheduler invariants

- no runnable thread disappears silently;
- sleeping thread cannot be selected as runnable;
- exited threads release resources;
- scheduler data structures remain consistent during preemption.

---

## 24. Process and thread model

Each process conceptually owns:

- address space;
- process identifier;
- security identity;
- open handles/resources;
- environment/configuration;
- one or more threads.

Each thread owns:

- thread identifier;
- execution context;
- scheduling state;
- stack;
- process association.

Lifecycle:

```text
CREATED → READY → RUNNING → BLOCKED → READY → EXITED
```

Failure transitions must be explicit.

---

## 25. IPC

IPC must support safe communication without allowing direct arbitrary memory access between unrelated processes.

Target mechanisms may include:

- message queues;
- pipes;
- shared memory with explicit mapping permissions;
- events/signals;
- handles/capabilities.

GUI services, Package Manager, Settings and other platform components should use documented IPC APIs rather than private memory hacks.

---

# PART V — USERSPACE AND SYSTEM CALLS

## 26. Syscall ABI

The syscall boundary is a versioned ABI.

Required families eventually include:

- process/thread control;
- memory operations;
- file/handle I/O;
- time;
- IPC;
- networking;
- device/service interfaces;
- configuration/service access.

Every syscall must define:

- number/identifier;
- argument types;
- return convention;
- error model;
- privilege requirements;
- pointer validation;
- compatibility policy.

The kernel must validate all userspace-provided addresses, lengths, handles and object references before dereferencing or using them.

---

## 27. Init and service manager

The init/service manager is the first major userspace authority.

Responsibilities:

- start essential services;
- honor service dependencies;
- restart recoverable services;
- record service failures;
- coordinate shutdown/reboot;
- avoid starting optional applications unless configured.

Service lifecycle:

```text
DECLARED
  ↓
STARTING
  ↓
RUNNING
  ↓
STOPPING
  ↓
STOPPED
```

Failure state must include restart limits/backoff so a crashing service cannot create an infinite restart loop.

---

## 28. Userspace application model

Applications run outside the kernel.

Applications obtain privileged functionality through:

- syscalls;
- service APIs;
- approved device/service interfaces.

The GUI toolkit must not grant raw hardware access by default.

---

## 29. ELF loading

The initial executable format can be ELF for the x86_64 userspace environment.

Loader responsibilities:

- validate ELF header;
- validate machine/type;
- validate program-header bounds;
- validate segment alignment;
- map loadable segments;
- enforce permissions;
- establish entry point;
- establish user stack;
- pass process startup arguments/environment;
- refuse malformed or unsafe inputs.

No executable file may cause arbitrary kernel memory mapping through unchecked header fields.

---

# PART VI — STORAGE, FILESYSTEM AND INSTALLATION

## 30. Storage stack

Layer the storage architecture:

```text
Physical controller
  ↓
Block device
  ↓
Partition
  ↓
Filesystem driver
  ↓
VFS
  ↓
File/path API
  ↓
Applications
```

This prevents GUI or package code from becoming coupled to a particular disk controller.

---

## 31. VFS

The virtual filesystem provides common operations:

- open;
- close;
- read;
- write;
- seek;
- stat/metadata;
- create;
- delete;
- rename;
- directory enumeration;
- permissions;
- synchronization/durability.

Paths must be normalized and validated.

---

## 32. Filesystem requirements

A production filesystem must define:

- on-disk metadata;
- allocation structures;
- directory structure;
- permissions;
- timestamps;
- crash consistency;
- corruption detection/recovery strategy;
- maximum path/name sizes;
- encoding requirements;
- mount/unmount lifecycle.

The initial filesystem choice is an implementation decision that must be documented when frozen. The architecture must not depend on a filesystem-specific API.

---

## 33. User data versus system data

Data classes should be explicit.

```text
SYSTEM
USER
CACHE
TEMPORARY
LOG
PACKAGE_STATE
RECOVERY
UPDATE_STAGING
```

### 33.1 Cleanup rule

Only data explicitly classified as safe-to-clean may be removed automatically.

Never silently remove:

- user documents;
- unknown files;
- recovery-required data;
- credentials/keys;
- active package state.

---

## 34. Installer

The installer must present destructive operations clearly.

A disk operation must identify:

- target disk/device;
- existing partitions where detectable;
- operation to be performed;
- data-loss consequence;
- final confirmation.

Installation stages:

```text
Detect hardware
  ↓
Select installation target
  ↓
Validate plan
  ↓
Prepare storage
  ↓
Install base system
  ↓
Install bootloader
  ↓
Write initial configuration
  ↓
Validate installation
  ↓
Reboot into SB
```

The installer must not claim success until installation verification passes.

---

# PART VII — INPUT, DISPLAY AND GUI PLATFORM

## 35. Input subsystem

Abstract input from hardware.

Target devices:

- PS/2 keyboard where relevant;
- USB keyboards;
- USB mice;
- touchpads;
- touchscreens where later supported;
- other HID devices.

Input events should carry enough metadata for the GUI layer to interpret them consistently.

---

## 36. Display architecture

Start with the broadest safe display path available, then accelerate when the appropriate driver exists.

Conceptual hierarchy:

```text
Display hardware
  ↓
Driver
  ↓
Framebuffer / accelerated display API
  ↓
Compositor
  ↓
Window system
  ↓
Toolkit
  ↓
Applications
```

### 36.1 Framebuffer fallback

A generic framebuffer should allow a basic desktop path when feasible even if advanced acceleration is unavailable.

### 36.2 GPU acceleration

Accelerated drivers must be optional from the architectural point of view.

The desktop should degrade gracefully when only a fallback renderer is available.

---

## 37. GPU and high-performance hardware strategy

The architecture must support future drivers for:

- integrated graphics;
- AMD GPUs;
- Intel GPUs;
- NVIDIA consumer GPUs;
- NVIDIA professional GPUs;
- other accelerators as practical.

The user's long-term target includes high-end NVIDIA RTX-class systems and professional/server-grade accelerators. These are **future compatibility targets**, not proof of current support.

Driver support must be classified as:

```text
Not started
Prototype
Boot/Display capable
Basic acceleration
Functional desktop
Feature-complete for target
Hardware verified
Release supported
```

A Release image must not claim a higher status than actual verification.

---

## 38. GUI event system

The GUI platform is event-driven.

Events include conceptually:

- pointer move;
- pointer button;
- keyboard input;
- text input;
- window creation/destruction;
- resize;
- focus changes;
- timer events;
- display changes;
- notifications;
- system state changes.

No ordinary GUI operation should depend on busy loops polling hardware continuously.

---

## 39. GUI toolkit

Minimum primitives:

- Window/surface
- Panel/container
- Button
- Label
- Text input
- Select/drop-down
- Checkbox/toggle
- List
- Scrollable area
- Menu
- Dialog
- Notification
- Progress indicator
- Image/icon
- Tabs

Toolkit responsibilities:

- layout;
- input dispatch;
- focus;
- rendering;
- accessibility metadata;
- localization-aware sizing;
- DPI/scaling support;
- theme integration.

---

## 40. Compositor and window system

The compositor owns presentation of window surfaces.

The window system owns:

- window identity;
- position/size;
- focus;
- z-order;
- visibility;
- input routing;
- display/workspace association.

Applications cannot directly overwrite another application's surface.

---

## 41. Desktop applications

### 41.1 Settings

Owns user-facing system configuration.

### 41.2 File Manager

Provides normal filesystem navigation and file operations.

### 41.3 Terminal

Provides shell execution and advanced system access through approved APIs.

### 41.4 SB Store

Provides package/application browsing and installation using the same package transaction engine as CLI tools.

### 41.5 Other base applications

Additional apps should be added only when their inclusion is justified by core desktop usability, and optional apps should remain removable without damaging the base system.

---

# PART VIII — NETWORK

## 42. Network architecture

Target hierarchy:

```text
NIC driver
  ↓
Ethernet/link layer
  ↓
IPv4 / IPv6
  ↓
ARP / Neighbor Discovery
  ↓
Routing
  ↓
UDP / TCP
  ↓
DNS / DHCP
  ↓
Sockets
  ↓
Network service/manager
  ↓
GUI + CLI applications
```

### 42.1 Requirements

- Ethernet.
- IPv4.
- IPv6.
- Loopback.
- DHCP.
- Static addressing.
- DNS configuration.
- Routing.
- Socket interface.
- Network diagnostics.
- Firewall/policy architecture.

Wi-Fi support is a driver/platform expansion and must not be falsely implied by Ethernet support.

### 42.2 Network operation safety

Every network operation that can block must have:

- timeout;
- cancellation/abort semantics where possible;
- meaningful error state;
- retry policy;
- backoff policy.

A failed DNS query must never make the entire desktop hang indefinitely.

### 42.3 Network UI

Settings should show:

- interface state;
- connection state;
- address information;
- DNS state;
- diagnostics;
- connect/disconnect controls;
- advanced options.

The GUI must use a service/API rather than directly configuring NIC hardware.

---

# PART IX — SOFTWARE DISTRIBUTION

## 43. Package model

The package system exists to keep the base small while making additional software easy to install.

Conceptual package manifest fields:

```text
package identity
name
version
architecture
minimum SB version
dependencies
conflicts
provides
files/content
entry points
permissions/capabilities
integrity information
signature metadata
upgrade/migration information
```

The exact serialization format is an implementation decision, but the semantic fields above must be represented.

---

## 44. Repository metadata

Repository metadata must support:

- package discovery;
- version selection;
- dependency resolution;
- architecture compatibility;
- integrity verification;
- signature verification;
- repository provenance;
- update information.

Official package metadata must be authenticated.

---

## 45. Download manager

The downloader should optimize for:

- small metadata;
- compressed content;
- resumable transfer;
- local cache reuse;
- connection reuse;
- sensible concurrency;
- efficient mirrors/CDN when available later.

Never skip integrity verification for speed.

---

## 46. Package transaction engine

A package installation/update/removal is a transaction.

Conceptual state:

```text
PLANNING
  ↓
RESOLVING
  ↓
DOWNLOADING
  ↓
VERIFYING
  ↓
STAGING
  ↓
COMMITTING
  ↓
COMMITTED
```

Failure must transition to a recoverable state where possible.

### 46.1 GUI/CLI concurrency

GUI Store and CLI package commands use the same transaction backend.

There must be a single authoritative package database/state lock or transaction coordinator.

Example:

```text
CLI starts transaction
  ↓
Package transaction lock acquired
  ↓
SB Store attempts install
  ↓
Store receives BUSY/LOCKED state
  ↓
GUI shows "Another software operation is in progress"
  ↓
CLI commits/aborts
  ↓
Lock released
  ↓
Store refreshes metadata/state
```

The two interfaces must never independently modify package state.

### 46.2 Crash safety

If power fails during a transaction, the next boot must detect incomplete state and either:

- finish a validated commit;
- roll back to the previous valid state;
- enter recovery if neither safe path is possible.

---

## 47. SB Store

The Store is a GUI frontend, not a separate package backend.

User actions:

```text
Search
 ↓
View package
 ↓
Inspect size/dependencies/permissions
 ↓
Install
 ↓
Download
 ↓
Verify
 ↓
Transaction
 ↓
Success / recovery
```

Store UI must clearly show:

- installed state;
- available version;
- download size;
- installed size where known;
- dependencies;
- required restart/logout;
- security/signature state.

---

# PART X — SECURITY AND PRIVACY

## 48. Security model

Security enforcement occurs at privileged boundaries.

Minimum foundations:

- kernel/userspace separation;
- memory protection;
- syscall validation;
- file permissions;
- process identity;
- capability/privilege model;
- package signature verification;
- update integrity;
- service privilege separation;
- network policy/firewall;
- protected configuration.

---

## 49. Application isolation

Optional applications should not receive broad system privileges by default.

The long-term application model should support explicit capabilities such as:

```text
filesystem read
filesystem write
network
camera/device access
USB/device access
process control
system configuration
```

The exact capability mechanism can evolve, but privilege must be explicit rather than silently inherited.

---

## 50. Secrets and credentials

Secrets must not appear in:

- ordinary logs;
- crash reports;
- support reports;
- package metadata;
- screenshots generated by diagnostics.

Potential sensitive classes include:

- passwords;
- authentication tokens;
- private keys;
- session credentials;
- recovery secrets.

A secret-management system should eventually provide protected storage rather than leaving credentials in arbitrary configuration files.

---

## 51. Privacy

SB does not require telemetry merely because it is useful for developers.

Any future diagnostic/telemetry feature must specify:

- what data is collected;
- why it is needed;
- whether it is optional;
- retention period;
- transmission destination;
- redaction rules.

Diagnostic reports generated locally should be user-controlled.

---

# PART XI — ERRORS, CRASHES, BSOD, RSOD AND RECOVERY

## 52. Error identity system

Every significant user-facing failure should have a stable identifier.

Recommended structure:

```text
SB-<SUBSYSTEM>-<CLASS>-<NUMBER>
```

The exact final syntax may be frozen later, but identifiers must remain stable across localized text changes.

---

## 53. Normal error UX

Normal errors are ordinary failures that do not justify stopping the OS.

Example categories:

- application cannot open file;
- package download failed;
- network timeout;
- insufficient permissions;
- device unavailable;
- configuration rejected.

UI should show:

```text
What happened
Why it happened, when known
What was affected
Whether recovery occurred
What the user can do
Error ID
[Details]
[Support Report]
```

The default view remains understandable to a normal user.

---

## 54. Kernel panic / BSOD

BSOD means:

> **The system was operating, but continuing is no longer safe.**

Use it for severe kernel/system state where safe continuation cannot be guaranteed.

A BSOD should contain, where available:

- stable error ID;
- failure class;
- exception/vector;
- hardware error code;
- instruction pointer;
- relevant register state;
- current CPU;
- current process/thread;
- subsystem;
- kernel version/build;
- boot/session ID;
- dump status;
- recovery/reboot status;
- support identifier.

The user-facing explanation and technical details must be separate levels.

---

## 55. RSOD

RSOD means:

> **The normal display/recovery path itself cannot be trusted, or a critical early-boot/system-integrity failure prevents safe use of the normal error environment.**

RSOD is intentionally rare.

It may be used when:

- normal graphics cannot be trusted;
- recovery display cannot be safely initialized;
- critical early-boot integrity is broken;
- necessary boot/recovery data is unusable.

It must not be used for an ordinary process crash.

---

## 56. Diagnostic data

Support reports should contain enough information to help a user submit a useful report or take the machine to repair/support.

Possible sections:

```text
System identity
SB version/build
Boot/session ID
Hardware summary
CPU summary
RAM summary
Display/GPU summary
Storage summary
Network summary
Loaded services/drivers
Recent relevant logs
Recent errors
Crash information
Package transaction history
Recovery state
```

Sensitive data is redacted before export.

---

## 57. Crash dumps

Crash dumps should be structured and versioned.

A dump format must support:

- architecture version;
- kernel build ID;
- thread/process state;
- exception information;
- stack/context;
- loaded module metadata;
- relevant memory ranges when safe.

Dump size must have bounded behavior.

A failure to save a dump must not cause a second fatal failure.

---

## 58. Recovery mode

Recovery should provide, as the architecture matures:

- boot diagnostics;
- filesystem checks;
- package transaction recovery;
- update rollback;
- configuration restoration;
- driver disablement;
- log export;
- support report generation;
- safe reboot/shutdown.

Recovery must minimize write operations to damaged storage.

---

# PART XII — UPDATE, BACKUP AND DATA MIGRATION

## 59. Update system

Target update lifecycle:

```text
Check metadata
 ↓
Verify source/signature
 ↓
Resolve dependencies
 ↓
Download
 ↓
Verify contents
 ↓
Stage
 ↓
Pre-activation validation
 ↓
Activate atomically
 ↓
Post-update health check
 ↓
Commit
```

### 59.1 Rollback

A failed update must have a defined rollback path for components where rollback is technically feasible.

### 59.2 Interrupted update

On next boot:

- detect staging/incomplete transaction;
- do not assume the update succeeded;
- verify current state;
- recover to a known-valid state.

---

## 60. Configuration migration

Every persistent configuration schema must have a version.

A migration step should be:

```text
old schema
  ↓
validate
  ↓
transform
  ↓
validate new schema
  ↓
atomically activate
```

Failed migration must preserve the old known-good state.

---

## 61. User backup/restore

The system should eventually expose a controlled mechanism to back up user configuration/data without requiring raw filesystem knowledge.

Backup must distinguish:

- user files;
- application configuration;
- system configuration;
- secrets;
- package state.

Secrets require special handling and must not be exported casually.

---

# PART XIII — POWER, TIME, AUDIO AND OTHER SYSTEM SERVICES

## 62. Power management

Target functionality:

- shutdown;
- reboot;
- suspend where supported;
- hibernate where later supported;
- display sleep;
- idle behavior;
- battery state;
- thermal information where available;
- performance/energy policy.

Power actions must be coordinated with userspace services and open transactions.

---

## 63. Time and region

Separate:

- monotonic time;
- wall clock;
- RTC;
- timezone;
- locale date/time presentation.

Settings must support region/timezone selection independently from UI language.

---

## 64. Audio

The architecture should support an audio service between hardware drivers and applications.

Target capabilities:

- output devices;
- input devices;
- volume;
- mute;
- per-application routing where practical;
- device selection;
- notifications/system sounds.

Applications must not directly program arbitrary audio hardware.

---

## 65. Clipboard and notifications

These are userspace platform services.

Clipboard must apply ownership/lifetime rules so an application cannot retain data unexpectedly forever.

Notifications must support:

- severity;
- title/body;
- source application;
- timestamp;
- user actions;
- dismissal;
- persistence rules.

---

# PART XIV — USB, PCI, DRIVERS AND HOTPLUG

## 66. PCI

PCI/PCIe discovery provides foundational hardware identification.

Responsibilities:

- enumerate bus/device/function;
- read configuration space safely;
- identify vendor/device/class;
- assign or consume resources as required;
- hand devices to matching driver frameworks.

Driver code must not assume that every discovered device is safe or supported.

---

## 67. Driver framework

Drivers should implement defined interfaces such as:

```text
probe
initialize
start
stop
suspend
resume
remove
interrupt handling
capability reporting
```

A driver failure should be isolated where possible.

### 67.1 Driver state

```text
DISCOVERED
 ↓
MATCHED
 ↓
INITIALIZING
 ↓
ACTIVE
 ↓
SUSPENDED
 ↓
REMOVED/FAILED
```

### 67.2 Driver trust boundary

Drivers are privileged and therefore high risk.

The project should prefer small, auditable driver interfaces and isolate policy from low-level hardware access.

---

## 68. USB

Target layers:

```text
USB host controller
  ↓
USB core
  ↓
HID/storage/network/audio/etc. class driver
  ↓
Device service
```

Hotplug events must not deadlock device management.

A device being unplugged must result in deterministic resource cleanup.

---

# PART XV — MINECRAFT-FIRST OPTIMIZATION WITHOUT LOSING GENERALITY

## 69. Minecraft product goal

SB is Minecraft-focused but not Minecraft-exclusive.

Minecraft-specific optimization must be implemented through well-defined policy/runtime layers.

### 69.1 Target workload categories

- Minecraft Java Edition client;
- Minecraft Server;
- modded Minecraft;
- Java/JVM workloads related to Minecraft;
- asset/resource processing;
- server management tools.

### 69.2 Possible optimization surfaces

- scheduler policy;
- CPU affinity/pinning policy;
- memory policy;
- filesystem/cache placement;
- network queue policy;
- I/O prioritization;
- JVM/runtime tuning;
- process priority;
- thermal/performance policy;
- background-service suppression while gaming.

No optimization is accepted without measurement.

---

## 70. JVM / Java runtime integration

The OS should provide an explicit runtime-management layer rather than hard-coding one Java installation throughout the OS.

Capabilities may include:

- multiple JVM versions;
- per-game instance JVM selection;
- JVM memory settings;
- GC/runtime configuration;
- launch parameters;
- compatibility checks;
- runtime download/install through the package infrastructure.

The exact bundled JVM distribution is a future packaging/legal/technical decision.

---

## 71. Minecraft Instance Manager

A future SB application/service can manage:

- instances;
- versions;
- mod loaders;
- mods/resource packs where legally/user-supplied;
- saves/worlds;
- Java runtime selection;
- launch options;
- logs/crash data.

It must use normal filesystem and package APIs rather than bypassing system security.

---

## 72. Minecraft Server Manager

Target functions:

- create server instance;
- configure memory/runtime;
- start/stop/restart;
- logs;
- backups;
- update workflow;
- resource monitoring;
- port/network configuration;
- scheduled operation;
- failure recovery.

The service must respect normal OS security and account permissions.

---

## 73. Performance benchmark suite

The project should maintain reproducible benchmarks for:

- boot;
- memory;
- CPU scheduling;
- storage I/O;
- network;
- JVM startup;
- Minecraft launch where legally/testably available;
- server tick behavior where suitable.

Optimization changes should compare before/after measurements rather than anecdotal speed claims.

---

# PART XVI — DEVELOPMENT ENVIRONMENT AND PROJECT INFRASTRUCTURE

## 74. Supported development environment

The project is intended to be developable through browser/cloud infrastructure when a local PC is unavailable.

A reproducible Codespaces/dev-container setup should eventually provide:

- compiler/toolchain;
- assembler;
- linker;
- GRUB/boot tooling where required;
- ISO tooling;
- QEMU;
- test utilities;
- repository build scripts.

The development environment must be defined as code rather than relying on undocumented manual installation steps.

---

## 75. Build system

Required build concepts:

```text
make
make clean
make check
make test
make iso
make run
```

Exact targets may vary, but there must be one obvious documented path for:

- clean build;
- ISO build;
- validation;
- QEMU boot;
- test execution.

### 75.1 Compiler assumptions

The build must explicitly establish:

- target architecture;
- freestanding mode;
- ABI;
- warning policy;
- optimization policy;
- relocation model;
- runtime helper expectations.

---

## 76. Continuous Integration

CI should validate progressively.

Minimum conceptual stages:

```text
Source checkout
 ↓
Dependency/toolchain setup
 ↓
Compile
 ↓
Assemble
 ↓
Link
 ↓
ISO
 ↓
Image validation
 ↓
QEMU boot
 ↓
Serial log assertions
 ↓
Subsystem self-tests
 ↓
Artifact upload
```

As the desktop matures add:

- userspace tests;
- filesystem tests;
- package tests;
- GUI startup tests;
- first-run tests;
- localization tests;
- persistence tests;
- recovery tests;
- upgrade tests.

CI must not pass by deleting the failing assertion.

---

## 77. QEMU test strategy

QEMU is a primary development target, not proof of universal hardware compatibility.

Test scenarios should include:

- normal boot;
- low-memory configuration;
- larger-memory configuration;
- malformed/edge input where injectable;
- storage failure simulation;
- network failure simulation;
- device absence;
- controlled exception;
- package transaction interruption;
- first-run configuration.

Timeouts must distinguish:

- expected idle state;
- actual hang;
- controlled shutdown.

A blanket timeout should not convert every hang into a successful test.

---

# PART XVII — HARDWARE COMPATIBILITY AND RELEASE VARIANTS

## 78. Compatibility model

SB should start with generic x86_64 systems and expand through capabilities.

Compatibility is classified independently for:

- boot;
- CPU;
- memory;
- storage;
- display;
- GPU acceleration;
- network;
- audio;
- input;
- power management;
- suspend/hibernate;
- multi-monitor;
- USB/hotplug.

A machine can be "boot supported" while not being "fully accelerated supported".

---

## 79. Release profiles

After the common platform is stable, variants may be produced when there is a measured reason.

Potential profiles:

```text
Generic
Gaming
Workstation
High-performance GPU
Professional/Creator
Server-oriented
```

These should share the common base rather than becoming unrelated OS forks.

---

## 80. Future high-end/server direction

The long-term architecture should permit systems with:

- many CPU cores;
- very large RAM;
- professional/high-end GPUs;
- multiple storage devices;
- high-speed networking;
- accelerator devices.

The desktop project remains the active scope now. Server/data-center specific product work is a later expansion of the same foundations, not a separate current implementation track.

---

# PART XVIII — SOFTWARE LIFECYCLE

## 81. Application lifecycle

Application installation and removal must be safe.

An application removal must not remove shared components that another installed package still requires.

Dependency removal decisions must be explicit and transactional.

---

## 82. Service lifecycle

Every long-running system service must specify:

- why it exists;
- startup dependency;
- whether it is mandatory;
- restart policy;
- resource budget;
- privileges;
- shutdown behavior;
- failure behavior;
- logging category.

Unnecessary services should not be started by default.

---

## 83. Cache lifecycle

Caches must have:

- owner;
- purpose;
- maximum size or growth policy;
- eviction policy;
- safe rebuild behavior.

A cache must never become the only copy of user data.

---

# PART XIX — COMPLETE FAILURE MATRIX

## 84. Failure classes

| Class | Example | Expected response |
|---|---|---|
| Application | app cannot open file | dialog/log, app remains isolated |
| Service | network service crash | restart service, notify if needed |
| Device | optional USB device failure | isolate device, continue OS |
| Package | install transaction fails | abort/rollback transaction |
| Storage | filesystem corruption | isolate/recovery workflow |
| Userspace | critical service failure | restart/recovery escalation |
| Kernel | unrecoverable page fault | BSOD/panic |
| Display | compositor unavailable | restart display/recovery |
| Early boot | invalid memory map | controlled boot failure |
| Integrity | critical boot/recovery state compromised | RSOD or recovery path |

---

## 85. Severity levels

Recommended conceptual levels:

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

User-facing severity and developer-facing log severity are related but not identical.

---

# PART XX — DATA FORMATS AND VERSIONING

## 86. Versioned interfaces

The following must have explicit versioning/migration policy:

- syscall ABI;
- package metadata;
- persistent configuration;
- crash dump format;
- diagnostic report format;
- update transaction format;
- application/service IPC APIs where compatibility matters.

---

## 87. Stable identifiers

Stable IDs are required for:

- errors;
- settings;
- packages;
- services;
- processes/threads during a session;
- user-facing diagnostic categories;
- localization messages.

Changing display text must not change the underlying identifier.

---

# PART XXI — TEST PLAN

## 88. Unit-level testing

Where practical test:

- parsers;
- memory-range arithmetic;
- bitmap operations;
- path normalization;
- package dependency resolution;
- version comparison;
- config migration;
- locale resolution;
- error serialization;
- protocol parsers.

---

## 89. Kernel integration tests

At minimum test:

- PMM allocation/free;
- VMM map/unmap/translation;
- heap allocation/free;
- interrupt initialization;
- timer event;
- scheduler transitions;
- process creation;
- syscall dispatch;
- ELF validation/loading;
- userspace fault isolation.

---

## 90. End-to-end desktop tests

Final release candidate must validate:

1. clean boot;
2. reboot;
3. shutdown;
4. first-run popup;
5. language selection;
6. language persistence;
7. keyboard layout;
8. mouse/input;
9. display;
10. settings persistence;
11. filesystem operations;
12. network configuration;
13. terminal operation;
14. package installation;
15. package removal;
16. package update;
17. GUI/CLI package-lock behavior;
18. recoverable application error;
19. support-report creation;
20. controlled kernel fault diagnostic;
21. recovery mode;
22. rollback;
23. update interruption recovery;
24. clean install;
25. upgrade install;
26. QEMU boot;
27. supported real-hardware boot;
28. artifact/checksum/signature validation.

---

# PART XXII — RELEASE ENGINEERING

## 91. Release readiness levels

Use explicit levels:

```text
PLANNED
DESIGNED
SKELETON
PARTIAL
FUNCTIONAL
BUILD-VERIFIED
RUNTIME-VERIFIED
HARDWARE-VERIFIED
RELEASE-READY
```

A source file existing is not evidence of FUNCTIONAL state.

A passing compile is not evidence of RUNTIME-VERIFIED state.

A QEMU pass is not evidence of universal HARDWARE-VERIFIED support.

---

## 92. Release artifact set

A stable release should eventually publish:

- installation ISO;
- checksum data;
- signatures where implemented;
- release notes;
- supported hardware matrix;
- known limitations;
- installation guide;
- recovery guide;
- administrator/developer documentation as appropriate.

---

## 93. Official ecosystem

The project should use low-cost/public infrastructure where practical.

Potential official surfaces include:

- GitHub repository;
- GitHub Pages official website;
- release downloads;
- documentation;
- official X account;
- official Discord/community;
- issue tracker;
- discussions/changelogs.

The community identity is Suiram.

Social/community infrastructure is secondary to software integrity and must not distract active engineering from the OS itself.

---

# PART XXIII — PROJECT DOCUMENTATION ARCHITECTURE

## 94. Master document rule

`SB_OS_DESIGN.md` is the highest-level specification and project backup.

It must remain sufficient to answer:

- What is SB?
- Why is SB built?
- What does the finished OS contain?
- What does the user see?
- How does the system boot?
- How is memory managed?
- How do processes work?
- How does storage work?
- How does networking work?
- How does GUI work?
- How do packages work?
- How are errors handled?
- How is recovery performed?
- How is the OS verified?
- What is the release process?
- How can a new developer/AI continue the project?

Individual `docs/*.md` files may describe subsystems in greater implementation detail, but they must not silently redefine product behavior.

When a lower-level document conflicts with this file, the conflict must be resolved explicitly rather than being ignored.

---

# PART XXIV — IMPLEMENTATION ORDER

## 95. Required vertical development path

Build in dependency order.

```text
0. Reproducible toolchain/build
1. Boot entry
2. Early CPU state
3. Early output
4. Boot information validation/protection
5. Kernel layout
6. PMM
7. VMM
8. Kernel heap
9. GDT/TSS/IDT
10. Exceptions
11. Interrupt controller
12. Timer abstraction
13. Synchronization
14. SMP groundwork
15. Scheduler
16. Threads/processes
17. IPC
18. Syscalls
19. ELF/userspace
20. Init/service manager
21. Storage block layer
22. VFS/filesystem
23. Persistent configuration
24. Input
25. Display/framebuffer
26. GUI primitives
27. Compositor/window system
28. Desktop shell
29. Localization
30. First-run language setup
31. Settings
32. Network stack
33. Network manager
34. Terminal/shell
35. Package format/backend
36. Package transaction engine
37. SB Store
38. Security hardening
39. Diagnostics/recovery
40. Update/rollback
41. Driver expansion
42. GPU acceleration
43. Minecraft runtime layer
44. Performance suite
45. Hardware expansion
46. Full release validation
```

No later subsystem should be represented as finished merely because an earlier subsystem is missing if that later implementation depends on it for correct operation.

---

# PART XXV — CURRENT DEVELOPMENT STATE

## 96. Current target

Active target: **SB Desktop, x86_64, initially validated with QEMU.**

The project should proceed toward a generic desktop first and expand compatibility later.

## 97. Current known implementation checkpoint

At the latest verified development point represented by the repository history:

- ISO generation is implemented and has been used by CI.
- Multiboot2 image validation is implemented in CI.
- Early kernel boot reaches the kernel.
- PCI enumeration reaches completion in the smoke-test path.
- Storage initialization was intentionally kept out of the blocking critical path during bootstrap work.
- The next critical runtime blocker has been PMM initialization.

The current PMM implementation must not be considered complete until QEMU reaches the post-PMM assertions in CI.

The runtime path currently attempts, after PMM:

```text
PMM
 ↓
VMM self-test
 ↓
Kernel heap self-test
 ↓
GDT/TSS
 ↓
Scheduler
 ↓
Process/Syscall
 ↓
Userspace preparation
 ↓
Timer
```

This is an implementation checkpoint, not the final architecture. The full finished desktop requires all later phases in this document.

---

# PART XXVI — CURRENT PMM DEBUGGING CONTRACT

## 98. PMM diagnostic procedure

When PMM initialization fails:

1. Prove exact last emitted log line.
2. Separate function entry from each internal stage.
3. Verify linker addresses of stack/page tables/PMM metadata.
4. Verify the bitmap address does not overlap code, stack, page tables or other active memory.
5. Verify the supplied usable-memory range is legal under the active boot memory map.
6. Verify the PMM does not reinitialize over live allocation metadata.
7. Verify no PMM logging call depends on heap/locks.
8. Reduce initialization to a minimal deterministic path.
9. Run a focused QEMU test.
10. Only then add back full memory-map integration.

Never hide the PMM failure by removing its test.

---

# PART XXVII — AI AND DEVELOPER OPERATING CONTRACT

## 99. Required work loop

Every implementation agent must:

```text
Read SB_OS_DESIGN.md
 ↓
Inspect repository
 ↓
Inspect current implementation
 ↓
Inspect latest relevant CI
 ↓
Identify exact state
 ↓
Choose smallest correct next change
 ↓
Implement
 ↓
Build
 ↓
Runtime test
 ↓
Inspect logs
 ↓
Fix root cause
 ↓
Add/update tests
 ↓
Update this document when architecture changes
 ↓
Record actual state
```

### 99.1 AI must not

- invent an implementation state;
- call unverified hardware supported;
- turn planned features into false completion claims;
- silently remove tests;
- replace real functionality with a visual mock and call it done;
- change persistent formats without a migration plan;
- add arbitrary privileged shortcuts;
- create a second package backend for the GUI;
- make normal errors fatal;
- use RSOD/BSOD for ordinary failures;
- silently delete user data;
- introduce a dependency merely for convenience when it materially increases the base image or runtime footprint without benefit.

---

# PART XXVIII — PERFORMANCE AND RESOURCE BUDGETING

## 100. What “lightweight” means

Track measurable metrics.

### Boot

- firmware-to-bootloader time where measurable;
- bootloader-to-kernel;
- kernel-to-userspace;
- userspace-to-desktop;
- first-frame latency.

### Runtime

- idle RAM;
- idle CPU;
- background wakeups;
- number of always-running services;
- GUI frame latency;
- disk footprint;
- package cache size;
- update download size.

### Optimization rule

Every optimization must identify:

- baseline;
- measurement method;
- change;
- new measurement;
- correctness/security tradeoff if any.

“Faster” without measurement is not an accepted engineering result.

---

# PART XXIX — COMPLETE USER-DATA SAFETY MODEL

## 101. Deletion safety

Before any destructive operation:

1. identify target;
2. classify data;
3. determine whether data is user-owned/system-owned/cache;
4. show consequence when user action is destructive;
5. require confirmation for consequential UI operations;
6. create recoverable state where appropriate;
7. perform operation transactionally where feasible;
8. report result.

### 101.1 Automatic cleanup

Safe automatic cleanup requires explicit rules.

Example eligible classes:

- expired temporary data;
- obsolete package cache;
- orphaned generated build artifacts when explicitly designated;
- stale update staging after safe recovery.

Unknown data is not a cleanup target.

---

# PART XXX — CHANGE CONTROL

## 102. Architectural decision record discipline

Important decisions should record:

- decision;
- alternatives considered;
- reason;
- compatibility impact;
- performance impact;
- security impact;
- migration requirements;
- affected documents/modules.

This information may be stored in individual ADR files, but the master specification should summarize decisions that materially change the product identity.

---

## 103. Backward compatibility

Before changing:

- persistent data;
- package metadata;
- syscall ABI;
- public application APIs;
- configuration schemas;
- crash dump formats;

define whether the change is:

```text
compatible
migratable
breaking but planned
unsupported experimental
```

---

# PART XXXI — COMPLETE DEFINITION OF DONE

## 104. SB Desktop v1 is not finished until

A normal user can:

1. install SB on a supported machine;
2. boot it without terminal commands;
3. reach a graphical desktop;
4. see the graphical first-run language selector when required;
5. select 日本語, English, 中文 or Español;
6. use a suitable keyboard layout independently of UI language;
7. complete basic setup;
8. reboot and retain settings;
9. use keyboard and mouse;
10. use the display system;
11. manage files;
12. configure networking;
13. use the terminal;
14. open Settings;
15. install optional software from SB Store;
16. manage the same packages through CLI;
17. update/remove software safely;
18. recover from supported application/service/package failures;
19. generate useful diagnostic/support reports;
20. understand serious failures;
21. enter recovery mode when required;
22. receive safe update/rollback behavior;
23. operate without unnecessary optional services consuming base resources.

And the project must have evidence for:

- QEMU runtime tests;
- clean-install tests;
- persistence tests;
- package transaction tests;
- update/rollback tests;
- supported real-hardware tests;
- release artifact integrity.

---

# PART XXXII — MASTER CHECKLIST FROM ZERO TO RELEASE

## 105. Build foundation

- [ ] Toolchain pinned/documented
- [ ] Freestanding kernel build
- [ ] Assembly architecture verified
- [ ] Linker script validated
- [ ] ISO reproducible enough for CI
- [ ] QEMU invocation documented
- [ ] Serial diagnostics available
- [ ] CI artifact retention configured

## 106. Boot

- [ ] Bootloader entry
- [ ] CPU state
- [ ] stack
- [ ] boot information validation
- [ ] memory reservation
- [ ] boot failure diagnostics

## 107. Kernel foundation

- [ ] PMM
- [ ] VMM
- [ ] heap
- [ ] GDT/TSS/IDT
- [ ] exception handling
- [ ] interrupts
- [ ] timer
- [ ] synchronization

## 108. Execution

- [ ] SMP groundwork
- [ ] scheduler
- [ ] threads
- [ ] processes
- [ ] IPC
- [ ] syscall ABI
- [ ] ELF/userspace
- [ ] init/service manager

## 109. Storage

- [ ] block layer
- [ ] partition handling
- [ ] filesystem
- [ ] VFS
- [ ] permissions
- [ ] configuration persistence
- [ ] installer
- [ ] recovery storage tools

## 110. Hardware

- [ ] PCI
- [ ] ACPI
- [ ] keyboard
- [ ] mouse/HID
- [ ] USB
- [ ] display/framebuffer
- [ ] audio
- [ ] network
- [ ] power
- [ ] GPU abstraction

## 111. GUI

- [ ] rendering
- [ ] text
- [ ] fonts
- [ ] localization
- [ ] event system
- [ ] windows
- [ ] compositor
- [ ] toolkit
- [ ] desktop shell
- [ ] Settings
- [ ] File Manager
- [ ] Terminal
- [ ] notifications
- [ ] clipboard
- [ ] accessibility

## 112. Software platform

- [ ] package format
- [ ] repository metadata
- [ ] downloader
- [ ] verification
- [ ] transaction engine
- [ ] CLI package management
- [ ] SB Store
- [ ] GUI/CLI locking
- [ ] update system
- [ ] rollback

## 113. Diagnostics/recovery

- [ ] error IDs
- [ ] structured logging
- [ ] support report
- [ ] crash data
- [ ] BSOD
- [ ] RSOD
- [ ] recovery mode
- [ ] failure escalation

## 114. Minecraft ecosystem

- [ ] JVM integration
- [ ] Minecraft runtime management
- [ ] instance management
- [ ] server management
- [ ] benchmark suite
- [ ] performance policies

## 115. Release

- [ ] clean install
- [ ] upgrade
- [ ] update interruption recovery
- [ ] supported hardware matrix
- [ ] performance regression checks
- [ ] security review
- [ ] documentation
- [ ] checksum/signature
- [ ] release artifacts
- [ ] official website/docs

---

# PART XXXIII — RECOVERY IF PROJECT CONTEXT IS LOST

## 116. Complete recovery procedure

If this project is handed to a new person or AI with no conversation history:

1. Read this entire `SB_OS_DESIGN.md`.
2. Read the repository README.
3. Inspect the repository tree.
4. Read the individual subsystem docs referenced by the tree.
5. Run the build.
6. Run the existing CI/QEMU path.
7. Inspect the latest logs.
8. Identify the highest-priority failed prerequisite.
9. Do not skip ahead just because later source files already exist.
10. Compare implementation to this specification.
11. Record discrepancies.
12. Fix the earliest blocking dependency.
13. Re-run validation.
14. Continue through the implementation order.

The new developer/AI must assume nothing is complete until verified.

---

# PART XXXIV — EXTERNAL STANDARDS AND REFERENCE PRINCIPLES

## 117. Standards policy

SB should reuse stable, published standards rather than inventing protocols unnecessarily.

Examples of relevant external standards include:

- Multiboot2 for boot information where used;
- UEFI for modern firmware interfaces;
- ACPI for system configuration/power management;
- ELF for executable loading where used;
- IPv4/IPv6/DNS/DHCP/TCP/UDP networking standards;
- PCI/PCIe device conventions;
- USB class standards;
- established filesystem and package-signing concepts where appropriate.

External standards are dependencies of the implementation, not substitutes for an SB architecture.

---

# PART XXXV — IMPORTANT NON-DECISIONS

## 118. Things intentionally not frozen yet

The following must not be invented merely to fill space. They become fixed only after engineering review and testing:

- final filesystem format;
- final package serialization format;
- final package repository protocol;
- final GUI theme/visual language;
- exact window/compositor protocol;
- final application sandbox mechanism;
- final update slot mechanism;
- final cryptographic key-management infrastructure;
- final choice of bundled JVM distribution;
- final NVIDIA/AMD/Intel driver strategy for every GPU generation;
- final release profile split.

When one of these is decided, document:

1. chosen design;
2. reason;
3. alternatives;
4. compatibility implications;
5. migration path if needed;
6. implementation status.

Never present an undecided implementation detail as a completed product requirement.

---

# PART XXXVI — MASTER PRINCIPLE

## 119. The SB standard

The finished SB should embody one consistent idea:

> **The base system should contain only what it genuinely needs; everything else should be easy to add, easy to configure, easy to remove safely, and easy to understand.**

At the same time:

> **Freedom does not mean chaos. The system must have stable interfaces, clear ownership, reliable diagnostics, strong defaults, controlled privileges and recoverable state.**

And:

> **Performance must come from good architecture and measurement, not from deleting correctness.**

And:

> **Minecraft optimization must make SB better for its primary workload without making SB cease to be a useful general desktop operating system.**

And most importantly:

> **This document exists so that the project does not depend on one conversation, one developer, or one AI remembering what SB was supposed to become.**

The implementation may change. The specification may be refined. But any change to what SB fundamentally is must be deliberate, recorded and verified.

---

# Appendix A — Canonical current scope

**Active:** SB Desktop.

**Architecture:** x86_64 first.

**Primary development environment:** repository + CI + QEMU; browser/cloud development is acceptable when locally applicable.

**Kernel languages:** C + x86_64 Assembly.

**Primary normal UI:** GUI desktop.

**Required advanced UI:** Terminal/CLI.

**Initial UI languages:** Japanese, English, Chinese, Spanish.

**First-run language UI:** graphical popup with select/drop-down control.

**Initial compatibility target:** generic x86_64 PC / QEMU.

**Long-term compatibility:** broad consumer, gaming, workstation and high-performance GPU hardware; server/data-center expansion later.

**Base philosophy:** minimal core, optional components, fast verified downloads, detailed usable settings, strong diagnostics, safe cleanup, recoverability and user choice.

**Current implementation blocker:** PMM initialization must be proven in CI before the memory layer can be considered verified.

---

# Appendix B — Current state notation

When updating this document or reporting progress, use exact language such as:

- `planned`
- `designed`
- `implemented but untested`
- `build verified`
- `QEMU verified`
- `real hardware verified`
- `release ready`

Avoid vague phrases such as:

- "basically done";
- "should work";
- "supported" without test scope;
- "finished" when only the source exists.

---

# Appendix C — Reference sources

External standards referenced in this specification should always be checked against current official publications when implementation depends on version-specific behavior.

- Multiboot2 Specification: GNU GRUB documentation. citeturn441644search30
- UEFI Specifications: UEFI Forum. Current published UEFI specification information is available from the official specifications pages. citeturn441644search0turn441644search2
- ACPI Specifications: UEFI Forum. Current published ACPI specification information is available from the official specifications pages. citeturn441644search0turn441644search13
