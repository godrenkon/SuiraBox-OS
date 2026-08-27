# SuiraBox OS — FINAL MASTER SPECIFICATION

> **Document type:** Product constitution + complete system specification + architecture blueprint + implementation plan + recovery backup + AI handoff reference.
>
> **This document is the authoritative description of what SB Desktop is intended to be.** It exists so that the project can survive loss of conversation history, loss of a developer, change of AI, migration of development environments, and long periods without active work.
>
> **Active product:** SB Desktop.
>
> **Current exclusion:** An independent CUI-only operating-system edition is not being developed in this phase. The GUI edition must still include a real terminal and CLI.
>
> **Status rule:** Nothing written here proves implementation. Source code, builds, tests, CI, QEMU and real-hardware tests determine actual status.

---

# 0. PURPOSE AND OPERATING RULE OF THIS FILE

## 0.1 What this file must accomplish

A person or AI who has never seen the original project conversation must be able to read this file and reconstruct:

- the purpose of SB;
- the intended user experience;
- the complete major feature set;
- the complete system architecture;
- the relationship between subsystems;
- the intended data flow;
- the intended lifecycle of important system state;
- failure and recovery semantics;
- the implementation sequence;
- what is deliberately optional;
- what is deliberately not part of the base system;
- what is undecided and therefore must not be guessed;
- what must be tested before a feature is called complete;
- what conditions define a releasable SB Desktop.

## 0.2 Single-master principle

`SB_OS_DESIGN.md` is the single top-level specification and backup document.

A detailed document under `docs/` may expand implementation detail, but it must not silently redefine the product.

If a lasting architectural decision changes, update this file.

## 0.3 Evidence hierarchy

When determining reality:

1. Repository source and built artifacts = what exists.
2. Tests, CI and runtime output = what has been verified.
3. This file = what the project is intended to become.
4. Other documentation, issues and conversations = supporting history.

This distinction is mandatory.

## 0.4 Feature state vocabulary

Use the following state model:

- `IDEA`: concept only.
- `SPECIFIED`: behavior/architecture documented.
- `SKELETON`: code structure exists.
- `PARTIAL`: some functionality works.
- `FUNCTIONAL`: core end-to-end behavior works in a controlled environment.
- `RUNTIME-VERIFIED`: automated runtime tests pass.
- `HARDWARE-VERIFIED`: real supported hardware passes.
- `RELEASE-READY`: all release criteria for the target release pass.

A source file existing does not make a feature functional.

---

# PART I — WHAT SB IS

# 1. PRODUCT IDENTITY

## 1.1 Names

- Project: **SuiraBox OS**
- Short name: **SB**
- Desktop product: **SB Desktop**
- Project/community identity: **Suiram**
- Current repository: `godrenkon/SuiraBox-OS`

## 1.2 Mission

SB exists to create an open-source desktop OS that removes as much unnecessary complexity, resource waste and forced behavior as practical while preserving power, compatibility and usability.

The target is not the smallest possible kernel at any cost. The target is a **small, fast, understandable and extensible complete system**.

## 1.3 Core product idea

SB should feel closer to a flexible toolkit than to a monolithic operating system that permanently installs everything for everyone.

Conceptual lifecycle:

```text
Minimal base
    ↓
Detect hardware/capabilities
    ↓
Choose what the user needs
    ↓
Download only required components
    ↓
Verify
    ↓
Install transactionally
    ↓
Integrate automatically
    ↓
Use
    ↓
Change/remove components later
```

This is the project's “RPG-maker-like” philosophy: build the system you need instead of being forced to accept every possible feature.

## 1.4 What this philosophy does NOT mean

SB must not become a collection of thousands of meaningless micro-packages.

A component should be separable only when separation produces a real benefit in one or more of:

- base installation size;
- RAM usage;
- startup time;
- security isolation;
- reliability;
- upgradeability;
- maintainability;
- user choice.

---

# 2. PRINCIPLES

## 2.1 Minimal by default

Only essential system functionality belongs in the base installation.

Optional software should be downloadable after installation.

## 2.2 User-oriented

Normal operations must be possible through GUI.

Advanced operations must remain possible through CLI.

## 2.3 Deep configuration without confusing UX

Settings can be very detailed, but the organization, search, descriptions and controls must remain understandable.

## 2.4 Honest behavior

SB must not pretend a feature works because a UI exists.

## 2.5 Explainable errors

An understandable error is preferable to a silent failure.

## 2.6 Recoverability

The smallest safe component should be restarted/recovered before escalating to whole-system failure.

## 2.7 Data preservation

Data integrity takes precedence over convenience.

Never silently delete user data.

## 2.8 Performance by measurement

Performance claims must be supported by benchmarks or measurements.

## 2.9 Standard interfaces where practical

Prefer open, documented and established interfaces for hardware and software interoperability.

## 2.10 Modularity without fragmentation

Use clean subsystem boundaries, not arbitrary layers for their own sake.

---

# PART II — THE FINISHED USER PRODUCT

# 3. COMPLETE USER EXPERIENCE

## 3.1 Normal boot

The final intended sequence is:

```text
Power on
 ↓
BIOS/UEFI firmware
 ↓
Bootloader
 ↓
Early kernel
 ↓
CPU / memory / interrupt foundation
 ↓
Hardware discovery
 ↓
Kernel runtime
 ↓
Userspace init
 ↓
Storage / configuration
 ↓
Display / input
 ↓
Compositor / window system
 ↓
Desktop shell
 ↓
First-run check
 ↓
First-run popup only when necessary
 ↓
Desktop ready
```

## 3.2 What a normal user must be able to do

At release maturity the GUI should support:

- launch applications;
- close applications;
- move, resize, minimize and maximize windows;
- switch windows/workspaces;
- create, move, copy, rename and safely delete files;
- browse storage;
- connect to supported networks;
- change display settings;
- change sound settings;
- change language;
- change keyboard layout;
- configure accounts;
- change privacy/security settings;
- install/remove/update applications;
- view update progress;
- recover supported failures;
- inspect understandable error details;
- create support reports;
- shut down/restart/sleep where supported;
- open a real terminal.

## 3.3 Advanced-user capabilities

Advanced users should be able to:

- manage packages from CLI;
- inspect processes and threads;
- inspect logs;
- inspect hardware;
- diagnose networking;
- diagnose storage;
- inspect performance;
- enter recovery tools;
- use developer tools;
- automate system operations.

---

# 4. FIRST BOOT

## 4.1 Requirement

The first normal graphical environment must not require terminal commands.

## 4.2 Required initial UI

A clean system should eventually display a small centered popup over the desktop:

```text
Welcome to SuiraBox

Language
[ English ▼ ]

[ Continue ]
```

Available initial choices:

- 日本語
- English
- 中文
- Español

## 4.3 First-run state

Persistent configuration stores a first-run completion state.

Conceptual states:

```text
UNINITIALIZED
 ↓
FIRST_RUN_ACTIVE
 ↓
CONFIGURING
 ↓
VALIDATING
 ↓
COMMITTING
 ↓
COMPLETE
```

Failure may return to `CONFIGURING` or `FIRST_RUN_ACTIVE`, but must not produce an infinite silent loop.

## 4.4 Language and keyboard layout

Display language and keyboard layout are separate properties.

Valid examples:

```text
Language = Japanese
Keyboard = US
```

or

```text
Language = English
Keyboard = JP
```

Locale may suggest a default keyboard layout, but the user remains able to change it.

Keyboard layout must be reliable before credential/text-heavy setup is required.

## 4.5 First-run write semantics

The selected state is validated, then committed atomically.

The sequence is:

```text
collect
 ↓
validate
 ↓
write temporary/new state
 ↓
fsync/commit as required
 ↓
atomic replace
 ↓
activate new configuration
 ↓
mark first run complete
```

An interrupted write must not create ambiguous configuration.

---

# 5. DESKTOP SHELL

The default shell should provide:

- application launcher;
- desktop/workspace surface;
- window/task management;
- system status area;
- notification mechanism;
- date/time;
- network indicator;
- audio indicator;
- power controls;
- Settings entry;
- File Manager entry;
- Terminal entry.

Optional shell extensions must use supported extension interfaces and must not require privileged access merely to render UI.

---

# 6. SETTINGS

Settings must be a real system application backed by a central configuration service.

Initial organizational model:

```text
System
Appearance
Display
Sound
Network
Keyboard & Mouse
Language & Region
Storage
Applications
Accounts
Privacy & Security
Performance
Power
Updates
Recovery
Developer
Accessibility
```

Every persistent setting has:

- stable identifier;
- type;
- valid range/enum;
- default;
- current value;
- validation rule;
- owning subsystem;
- persistence rule;
- localization key;
- schema version/migration rule where needed.

No duplicated source of truth.

---

# PART III — OS INTERNAL ARCHITECTURE

# 7. MASTER LAYER MODEL

```text
BIOS / UEFI
    ↓
Bootloader
    ↓
Early Architecture Layer
    ├─ CPU
    ├─ Stack
    ├─ Paging bootstrap
    └─ Boot information
    ↓
Kernel Foundation
    ├─ PMM
    ├─ VMM
    ├─ Heap
    ├─ GDT/TSS/IDT
    ├─ Exceptions
    ├─ Interrupts
    └─ Timers
    ↓
Kernel Runtime
    ├─ Scheduler
    ├─ Threads
    ├─ Processes
    ├─ IPC
    ├─ Syscalls
    ├─ Security
    └─ Logging/Panic
    ↓
Device / I/O
    ├─ PCI
    ├─ USB
    ├─ Storage
    ├─ Input
    ├─ Display/GPU
    ├─ Network
    ├─ Audio
    └─ Power
    ↓
Storage / VFS
    ↓
Userspace Init / Service Manager
    ↓
System Services
    ├─ Configuration
    ├─ Network
    ├─ Package
    ├─ Update
    ├─ Account
    ├─ Logging
    ├─ Notifications
    └─ Display/Input
    ↓
GUI Platform
    ├─ Text/Fonts
    ├─ Toolkit
    ├─ Compositor
    └─ Window System
    ↓
Desktop Shell
    ↓
System Applications
    ├─ Settings
    ├─ File Manager
    ├─ Terminal
    └─ SB Store
    ↓
Optional Applications
```

# 8. DEPENDENCY RULES

Lower layers must never depend on higher UI/application layers for core operation.

Examples:

- PMM does not depend on filesystem or GUI.
- VMM does not depend on Store.
- A driver does not depend on Settings widgets.
- Package transactions do not require the GUI.
- GUI cannot directly program hardware.
- Early panic diagnostics cannot require userspace.

---

# PART IV — BOOT AND PLATFORM

# 9. FIRMWARE

SB must support a platform abstraction covering conventional BIOS/legacy and UEFI boot paths as implementation matures.

Firmware-specific state is normalized before becoming a dependency of higher subsystems.

The project should consult current authoritative platform specifications rather than relying on remembered implementation details.

## 9.1 Current technical references

Use the applicable versions of:

- UEFI specification;
- ACPI specification;
- Intel/AMD architecture documentation;
- PCI/PCIe specifications;
- USB specifications;
- NVMe and storage specifications.

At the time of this specification, the UEFI Forum lists UEFI Specification 2.11, Intel publishes current Intel 64/IA-32 Architecture Software Developer Manuals, and PCI-SIG lists PCI Express Base Specification 7.0 as the current approved base specification. These versions can change and must be checked when implementing a feature that depends on them.

---

# 10. BOOTLOADER

Responsibilities:

- load kernel;
- load required modules;
- establish early CPU state;
- provide boot information;
- provide required memory/framebuffer information;
- establish/pass stack and page-table information as required;
- transfer control to kernel entry.

Not responsible for:

- GUI;
- ordinary filesystem management;
- package manager;
- settings;
- applications.

---

# 11. BOOT INFORMATION OWNERSHIP

Every boot-provided structure has a lifecycle:

```text
received
 ↓
validate
 ↓
protect source memory
 ↓
parse
 ↓
normalize/copy when necessary
 ↓
publish owned internal representation
 ↓
subsystems consume
 ↓
release source only after last consumer no longer needs it
```

This applies to:

- Multiboot2 information;
- memory map;
- framebuffer metadata;
- ACPI table data;
- boot modules;
- command line.

No allocator may reclaim a source range while it is still referenced.

---

# 12. BOOT STATE MACHINE

Conceptual states:

```text
RESET
EARLY_CPU
BOOT_INFO_READY
EARLY_CONSOLE_READY
MEMORY_BOOTSTRAP
MEMORY_READY
CPU_RUNTIME_READY
INTERRUPTS_READY
SCHEDULER_READY
USERSPACE_READY
STORAGE_READY
DISPLAY_READY
DESKTOP_READY
RECOVERY
PANIC
```

Transitions must be explicit and logged at debug/diagnostic level.

---

# PART V — CPU AND MEMORY FOUNDATION

# 13. IMPLEMENTATION LANGUAGE

The initial kernel is C-based.

x86_64 Assembly is used where direct machine-level control is necessary:

- boot entry;
- mode transitions;
- interrupt entry/exit;
- context-switch primitives;
- syscall entry;
- CPU-specific operations.

Do not use assembly when a safe, clear C implementation is sufficient.

---

# 14. CPU INITIALIZATION

The architecture layer establishes:

- known stack;
- GDT;
- TSS;
- IDT;
- paging state;
- interrupt state;
- CPU feature data;
- timer source;
- per-CPU state when SMP is active.

Optional CPU features require detection and fallback policy.

---

# 15. GDT / TSS / IDT

## GDT

Defines execution/data contexts required by the selected x86_64 model.

## TSS

Provides required task-state and privilege-transition stack support.

## IDT

Routes CPU exceptions and interrupts to controlled entry stubs.

Assembly-to-C handler ABI must be documented and consistent.

---

# 16. EXCEPTIONS

Exception handling must cover all relevant x86_64 architectural faults required by the implementation, including at minimum:

- divide error;
- debug/breakpoint handling;
- invalid opcode;
- general protection fault;
- page fault;
- double fault;
- invalid TSS;
- stack fault;
- alignment check when enabled;
- machine-check strategy;
- other enabled architectural exceptions.

For page faults, capture where possible:

- fault address;
- error code;
- instruction pointer;
- current process/thread;
- address-space identity;
- access classification.

A user-process fault should normally terminate/isolate that process rather than crash the whole OS, provided kernel integrity remains intact.

---

# 17. PHYSICAL MEMORY MANAGER — PMM

## 17.1 Purpose

PMM is the source of truth for physical page ownership.

## 17.2 Base unit

Initial x86_64 page size: 4 KiB.

Larger-page support is optional and must not invalidate the base 4 KiB model.

## 17.3 Conceptual page states

```text
UNKNOWN
RESERVED
FREE
ALLOCATED
RECLAIMABLE
FIRMWARE
DEVICE
```

## 17.4 Pages that must not be returned

At minimum:

- kernel image;
- active boot stack;
- active page tables;
- PMM metadata/bitmap;
- active Multiboot2 data;
- active UEFI data;
- active ACPI tables;
- active boot modules;
- firmware-reserved memory;
- device/MMIO ownership where represented as physical ranges;
- any explicitly owned boot allocation.

## 17.5 Initialization algorithm

Final design:

```text
receive normalized memory map
 ↓
mark all pages unavailable
 ↓
mark verified usable RAM free
 ↓
reserve kernel and boot memory
 ↓
reserve allocator metadata
 ↓
reserve still-live boot structures
 ↓
verify invariants
 ↓
publish PMM_READY
```

A bootstrap mode may use a fixed known-safe area during earliest development, but the final allocator must be memory-map-driven.

## 17.6 API contract

Conceptual functions:

```c
pmm_init(...)
pmm_add_usable_range(start, end)
pmm_reserve_range(start, end)
pmm_alloc_page()
pmm_free_page(page)
pmm_total_pages()
pmm_free_pages()
```

Each API must define:

- alignment;
- valid ranges;
- ownership;
- failure behavior;
- concurrency behavior;
- whether operation is allowed during interrupt context.

## 17.7 PMM invariants

- allocator metadata cannot overlap allocated pages;
- reserved pages never returned;
- double-free cannot corrupt state;
- invalid addresses do not corrupt state;
- counters never underflow;
- allocation returns page-aligned addresses;
- reinitialization does not erase ownership of active pages.

## 17.8 PMM logging rule

PMM diagnostics must never require the PMM or heap in order to work.

Early PMM logging must use a bounded low-level output path.

---

# 18. VIRTUAL MEMORY MANAGER — VMM

Responsibilities:

- create address spaces;
- map/unmap pages;
- control read/write/execute permissions;
- maintain kernel/user separation;
- integrate page faults;
- manage page-table memory;
- maintain translation state/TLB consistency.

## 18.1 Required protection

Userspace cannot arbitrarily:

- write kernel-only mappings;
- access another process's private mappings;
- modify page tables;
- bypass privilege checks.

## 18.2 Mapping lifecycle

```text
request
 ↓
validate virtual range/alignment
 ↓
validate physical ownership
 ↓
allocate required page-table objects
 ↓
install mapping
 ↓
update/invalidate translation state
 ↓
return result
```

Unmapping must account for both page-table and physical-page ownership.

---

# 19. KERNEL HEAP

The heap exists above page allocation.

Requirements:

- alignment guarantee;
- overflow checking;
- clear failure behavior;
- zero-size semantics;
- safe free;
- corruption diagnostics where practical;
- fragmentation metrics;
- no hidden recursion through logging.

Allocation classes should eventually distinguish ordinary allocations from physically contiguous/DMA-constrained allocations.

---

# PART VI — CONCURRENCY AND EXECUTION

# 20. SYNCHRONIZATION

Provide primitives appropriate to the kernel:

- atomic operations;
- spinlocks;
- interrupt-safe critical sections;
- mutex/sleeping locks;
- condition/wait mechanisms;
- reference counting;
- bounded lock-free structures where actually justified.

Rules:

- a lock usable in interrupt context must not sleep;
- lock ordering must be defined for nested locks;
- low-level locks must not indirectly invoke GUI code;
- logging must not cause lock recursion.

---

# 21. TIMER ARCHITECTURE

The scheduler uses a canonical timer API, not direct knowledge of one physical timer.

Early/fallback sources may include PIT; modern systems may use Local APIC timer, HPET or appropriate TSC-derived mechanisms depending on platform capabilities.

Transition:

```text
early timer
 ↓
platform detection
 ↓
modern interrupt/timer initialization
 ↓
canonical timer service
 ↓
scheduler / sleep / timeout users
```

Required timer semantics:

- monotonic clock;
- deadlines;
- sleep/wake;
- timeout;
- cancellation;
- scheduler tick/preemption source where applicable.

Timer source selection must be tested across hardware classes.

---

# 22. LOGGING

## Early logging

Works before heap, scheduler, storage, userspace and GUI.

## Runtime logging

Structured records may include:

- timestamp;
- severity;
- component;
- event/error ID;
- CPU;
- process/thread where available;
- boot/session ID.

## Interrupt/panic-safe logging

Must not:

- allocate;
- block;
- sleep;
- perform filesystem I/O;
- require GUI;
- wait for scheduler resources;
- acquire locks that may already be held.

Use a preallocated bounded buffer/ring when appropriate.

---

# 23. SCHEDULER

Responsibilities:

- choose runnable thread;
- manage runnable/blocked/sleeping state;
- perform context switching;
- cooperate with timer/preemption;
- account for CPU usage.

Initial policy should favor correctness and simplicity.

Later policy may support:

- priority classes;
- CPU affinity;
- load balancing;
- workload-aware policy;
- power-aware scheduling.

Scheduling policy remains independent of interrupt-controller implementation.

---

# 24. THREADS

Each thread has conceptual state:

```text
NEW
RUNNABLE
RUNNING
BLOCKED
SLEEPING
TERMINATING
DEAD
```

Each thread owns or references:

- ID;
- execution context;
- kernel stack;
- owning process;
- scheduling metadata;
- exit state;
- wait state.

---

# 25. PROCESSES

Each process includes:

- PID;
- address space;
- thread set;
- credentials;
- handles/open resources;
- parent/child relationships;
- current directory where applicable;
- exit state/code.

Process destruction must be idempotent from the perspective of repeated cleanup paths.

---

# 26. IPC

Provide a safe family of mechanisms such as:

- pipes;
- message queues;
- event objects;
- shared memory with explicit ownership/permissions;
- local sockets or equivalent service transport.

IPC must be bounded and resistant to trivial kernel-object exhaustion.

---

# 27. SYSCALL ABI

The userspace interface is versioned.

Conceptual syscall families:

```text
process/thread
memory
file/handle
IPC
time
network
device/service
configuration
identity/security
```

Every call validates:

- pointer;
- buffer length;
- integer overflow;
- handle lifetime;
- capability/permission.

The ABI is not allowed to depend on GUI implementation details.

---

# 28. ELF / USERSPACE LOADER

Loading sequence:

```text
open image
 ↓
validate architecture/file structure
 ↓
validate program headers and bounds
 ↓
create address space
 ↓
map segments
 ↓
apply permissions
 ↓
create user stack
 ↓
prepare argv/env/auxiliary data
 ↓
create initial thread
 ↓
transfer control
```

Reject:

- malformed headers;
- overflowed ranges;
- invalid alignment;
- overlapping unsafe segments;
- invalid entry points;
- unauthorized mappings.

---

# 29. INIT / SERVICE MANAGER

The initial userspace manager starts required services in dependency order.

Capabilities:

- dependency ordering;
- startup timeout;
- service state;
- restart policy;
- restart-storm protection;
- shutdown ordering;
- service logs;
- service health state.

Optional components remain lazy wherever possible.

---

# PART VII — STORAGE

# 30. STORAGE STACK

```text
Hardware/controller
 ↓
Driver
 ↓
Block device abstraction
 ↓
Partition layer
 ↓
Filesystem
 ↓
VFS
 ↓
File API
 ↓
Applications/services
```

Potential device backends include SATA/AHCI, NVMe and USB storage as support is added.

---

# 31. VFS

Required conceptual operations:

- open;
- close;
- read;
- write;
- seek;
- stat;
- create;
- mkdir;
- rename;
- remove/unlink;
- directory enumeration;
- permissions;
- timestamps;
- handles.

Path handling must define semantics for:

- `.`;
- `..`;
- mount boundaries;
- symbolic links if supported;
- invalid names;
- concurrent deletion/rename cases;
- permission errors.

---

# 32. FILESYSTEM SELECTION

The final on-disk filesystem is deliberately not hard-coded in this master document until technical evaluation is complete.

Selection criteria:

- reliability;
- recovery after power loss;
- implementation complexity;
- performance;
- tooling;
- license;
- portability;
- snapshot/rollback needs;
- large-file support;
- metadata integrity.

Do not invent a final filesystem choice just to make a roadmap look complete.

---

# 33. CONFIGURATION STORAGE

Configuration data must be:

- schema-based;
- versioned where persistent;
- validated;
- atomically updated;
- migratable;
- recoverable after interrupted writes.

System-critical configuration must have a safe default if corruption is detected.

---

# 34. DATA CLASSIFICATION

SB must distinguish at minimum:

```text
USER_DATA
SYSTEM_DATA
CONFIGURATION
PACKAGE_DATABASE
PACKAGE_CACHE
TEMPORARY
LOGS
CRASH_DATA
RECOVERY_DATA
DOWNLOAD_STATE
GENERATED_ARTIFACTS
```

Each class has a separate retention/cleanup policy.

Unknown files are not automatically treated as disposable.

---

# 35. INSTALLER

The installer is separate from the daily desktop but uses the same platform/storage concepts.

Required flow:

```text
Live/installer boot
 ↓
Hardware discovery
 ↓
Storage selection
 ↓
Partition decision
 ↓
Clear destructive confirmation
 ↓
Filesystem preparation
 ↓
Base system installation
 ↓
Boot configuration
 ↓
Initial configuration
 ↓
Verification
 ↓
Reboot
```

No irreversible action without clear confirmation.

The installer must verify completion rather than assume it.

---

# 36. RECOVERY ENVIRONMENT

Eventually provide an environment capable of:

- mounting filesystems;
- checking filesystem state;
- inspecting logs;
- reverting failed updates;
- restoring configuration;
- repairing boot configuration;
- exporting diagnostic/support data.

---

# PART VIII — HARDWARE AND DEVICE MODEL

# 37. DEVICE LIFECYCLE

A device transitions conceptually through:

```text
DISCOVERED
 ↓
IDENTIFIED
 ↓
RESOURCES_ASSIGNED
 ↓
DRIVER_BOUND
 ↓
ACTIVE
 ↓
QUIESCING
 ↓
DETACHED
```

Every driver must define attach, failure and detach behavior.

---

# 38. PCI

PCI discovery must safely enumerate bus/device/function space.

Later support must handle the capabilities actually required by drivers, potentially including:

- configuration space;
- BARs;
- interrupt routing;
- MSI/MSI-X;
- capability lists;
- power management;
- device reset where safe.

PCIe implementation must follow applicable PCI-SIG specifications rather than assumptions about one motherboard.

---

# 39. USB

USB design includes:

- host controller abstraction;
- device enumeration;
- descriptors;
- endpoints;
- transfer scheduling;
- hotplug;
- class-driver framework.

Class support is expanded according to actual product requirements.

---

# 40. INPUT

Abstract devices:

- keyboard;
- mouse;
- touchpad;
- touchscreens where supported;
- controller/game input where productized.

Input events carry enough state/order information for reliable GUI event processing.

---

# 41. DISPLAY AND GPU

Minimum graphics path:

```text
Display/framebuffer source
 ↓
Display service
 ↓
Surface/buffer abstraction
 ↓
Compositor
 ↓
Window system
 ↓
Desktop
```

Generic framebuffer is the fallback path where possible.

Hardware-accelerated graphics is layered above the minimum display path.

---

# 42. GPU COMPATIBILITY LEVELS

Never call a GPU “supported” without defining the level.

```text
DETECTED
↓
BASIC_FALLBACK
↓
DRIVER_PRESENT
↓
FUNCTIONAL
↓
ACCELERATED
↓
PERFORMANCE_VERIFIED
↓
HARDWARE_VERIFIED
```

Future targets include:

- Intel graphics;
- AMD graphics;
- NVIDIA consumer GPUs including RTX families;
- NVIDIA professional/workstation/data-center GPUs;
- other accelerators if justified.

A missing vendor driver must degrade gracefully if framebuffer/alternative paths are available.

---

# 43. AUDIO

Architecture:

```text
Audio hardware driver
 ↓
Audio device service
 ↓
Mixer / routing
 ↓
Application audio API
```

Applications should not need to program hardware codecs directly.

---

# 44. POWER MANAGEMENT

Capabilities may include:

- shutdown;
- reboot;
- suspend/sleep;
- hibernation if later implemented;
- display power states;
- battery reporting;
- CPU power policies;
- wake-source management.

Storage flushing and state consistency take precedence over speed of shutdown.

---

# 45. ACPI

ACPI provides platform information needed for:

- CPU topology;
- interrupt routing;
- power control;
- sleep/wake;
- thermal/platform data where safely supported.

ACPI table memory ownership must follow the same lifetime rules as other boot-provided data.

---

# 46. SMP

SMP progression:

```text
one CPU correct
 ↓
per-CPU data
 ↓
secondary CPU detection
 ↓
secondary CPU startup
 ↓
per-CPU scheduler
 ↓
IPI/inter-CPU control
 ↓
affinity/load balancing
```

Per-CPU state may include:

- current thread;
- scheduler state;
- interrupt nesting;
- kernel stack;
- local timer;
- statistics;
- CPU feature state.

---

# PART IX — GUI PLATFORM

# 47. TEXT AND FONT SYSTEM

GUI text handling must support Unicode-aware operation.

Requirements:

- safe string length handling;
- font fallback;
- glyph availability checks;
- multilingual layout;
- text measurement;
- long-string resilience;
- internationalization-safe UI geometry.

The initial language set requires adequate Japanese, English, Chinese and Spanish glyph coverage.

---

# 48. IME / INPUT METHODS

Input methods must be service-like rather than copied into every application.

Conceptual states:

```text
inactive
 ↓
composing
 ↓
candidate_selection
 ↓
commit
 ↓
inactive
```

The framework must support language switching and application focus semantics.

Japanese and Chinese input must be practical, not merely display-capable.

---

# 49. GUI EVENT SYSTEM

Every interactive GUI object follows a common event model.

Events include conceptually:

- pointer movement;
- pointer press/release;
- keyboard press/release;
- text input;
- focus change;
- window state change;
- timer callback;
- system notification.

Events must not be delivered to destroyed objects.

---

# 50. GUI TOOLKIT

Required reusable controls:

- label;
- text;
- button;
- checkbox;
- choice/radio control;
- select/drop-down;
- text field;
- password field;
- slider;
- progress bar;
- list;
- tree/list hierarchy;
- menu;
- dialog;
- notification;
- scroll container.

All controls use shared:

- theme;
- localization;
- accessibility;
- event routing;
- focus management.

---

# 51. COMPOSITOR / WINDOW SYSTEM

Responsibilities:

- surfaces;
- window lifecycle;
- focus;
- stacking order;
- clipping;
- damage tracking;
- input routing;
- presentation;
- multiple displays;
- workspace semantics.

An application failure should not automatically corrupt other application surfaces.

---

# 52. DESKTOP SERVICES

The desktop platform provides:

- notifications;
- clipboard;
- settings integration;
- launcher integration;
- application lifecycle;
- window management;
- system status;
- theme configuration.

---

# 53. CLIPBOARD

Clipboard contents are application data and must not be automatically logged.

The model must define:

- ownership;
- lifetime;
- supported formats;
- access control;
- clearing behavior.

---

# 54. ACCESSIBILITY

The platform should support:

- keyboard-only navigation;
- visible focus;
- scalable UI/text;
- semantic control names;
- reduced-motion setting where applicable;
- contrast/accessibility preferences;
- future screen-reader integration.

Accessibility belongs in the common GUI platform rather than being reinvented application by application.

---

# PART X — LOCALIZATION

# 55. INITIAL LANGUAGES

Initial product languages:

- 日本語
- English
- 中文
- Español

## 55.1 Message IDs

Every user-visible system/application string uses a stable message identifier.

Examples:

```text
ui.welcome.title
ui.first_run.language
settings.language
error.network.timeout
error.storage.read_failed
panic.kernel.page_fault
```

## 55.2 Fallback order

Conceptually:

```text
selected language
 ↓
configured fallback
 ↓
English
 ↓
stable technical identifier
```

Missing translation must never crash the GUI.

## 55.3 Language package model

Language data may be shipped separately from the minimal base image when this materially reduces base size.

---

# PART XI — NETWORK

# 56. NETWORK STACK

Conceptual structure:

```text
NIC driver
 ↓
Ethernet/link
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
Userspace network manager/applications
```

## 56.1 Required network behavior

- loopback;
- IPv4;
- IPv6;
- DHCP where supported;
- static addressing;
- DNS;
- routing;
- socket API;
- network diagnostics;
- firewall/policy layer.

The implementation may stage these over multiple releases.

---

# 57. NETWORK MANAGER

The central service owns:

- interfaces;
- profiles;
- addresses;
- routes;
- DNS state;
- DHCP state;
- connection status;
- retry policy.

GUI and CLI call the same service API.

Long operations require timeout and cancellation semantics.

---

# 58. FIREWALL / NETWORK POLICY

The final firewall system must have:

- deterministic rule ordering;
- clear defaults;
- safe update semantics;
- GUI/CLI management;
- diagnostics.

No secret data in network logs.

---

# PART XII — TERMINAL AND SOFTWARE DISTRIBUTION

# 59. TERMINAL

SB Desktop includes a genuine terminal.

Baseline requirements:

- command execution;
- environment variables;
- quoting/arguments;
- pipes;
- redirection;
- signals/control behavior;
- filesystem commands;
- process tools;
- network tools;
- package commands;
- diagnostics;
- recovery commands.

The terminal is an advanced surface, not the first-run interface.

---

# 60. PACKAGE FORMAT

Each package must conceptually have:

- package ID;
- version;
- target architecture;
- metadata;
- files;
- dependencies;
- conflicts;
- capabilities/provides;
- integrity hash;
- signature metadata where required;
- lifecycle hooks only when justified.

The exact archive/container format is deliberately undecided until evaluated.

---

# 61. PACKAGE DATABASE

The package database records:

- installed package identities;
- versions;
- ownership of files;
- dependencies;
- configuration-state markers;
- transaction history.

Database corruption must have detection/recovery behavior.

---

# 62. PACKAGE TRANSACTION ENGINE

Required lifecycle:

```text
REQUESTED
 ↓
RESOLVED
 ↓
LOCKED
 ↓
DOWNLOADING
 ↓
VERIFIED
 ↓
STAGED
 ↓
PREPARED
 ↓
COMMITTED
 ↓
ACTIVATED
```

Failure paths must preserve a known-consistent state.

---

# 63. GUI / CLI PACKAGE CONCURRENCY

The GUI Store and CLI use the same transaction coordinator.

If one transaction holds the package lock:

```text
other client requests package operation
 ↓
transaction coordinator reports BUSY
 ↓
client displays current operation
 ↓
client waits/retries/cancels according to policy
```

Neither client may bypass the transaction layer.

---

# 64. SB STORE

The graphical Store provides:

- search;
- categories;
- application/package details;
- version information;
- dependencies where useful;
- install;
- remove;
- update;
- download progress;
- retry/cancel;
- verification state;
- readable errors.

The Store does not contain a separate installation engine.

---

# 65. DOWNLOAD SYSTEM

Goals:

- compact metadata;
- compression;
- resumable transfers;
- local cache;
- range requests where supported;
- efficient mirrors/CDN later;
- concurrency where beneficial;
- checksum verification;
- signature verification.

Download speed must never bypass trust/integrity checks.

---

# PART XIII — SECURITY AND PRIVACY

# 66. PRIVILEGE MODEL

The OS must establish actual kernel/userspace privilege separation.

Privileged operations require:

- permission/capability validation;
- syscall boundary enforcement;
- audit/diagnostic treatment where useful.

GUI visibility is never the only security boundary.

---

# 67. ACCOUNTS

The final system requires:

- unique user identity;
- credentials abstraction;
- groups/roles or equivalent authorization;
- session;
- home directory;
- process credentials.

Exact authentication backend remains a technical selection to be documented before final implementation.

---

# 68. APPLICATION ISOLATION

The architecture should support progressive isolation through combinations of:

- filesystem restrictions;
- process separation;
- device capability grants;
- network policy;
- resource limits.

The exact sandbox technology remains an explicit design decision until kernel primitives and threat model are evaluated.

---

# 69. SECRET DATA

Secrets include:

- passwords;
- authentication tokens;
- private keys;
- equivalent credentials.

They must not appear in normal logs, support reports or crash summaries.

A secure secret-storage mechanism should exist before applications require persistent credentials.

---

# 70. SUPPLY-CHAIN SECURITY

Packages/updates eventually require:

- authenticated metadata;
- integrity hashes;
- signatures;
- trusted key policy;
- dependency verification;
- rollback/replay protections appropriate to the threat model.

Package names are never sufficient proof of authenticity.

---

# 71. THREAT MODEL

Consider at least:

- malicious application;
- malicious package;
- compromised repository;
- malicious network peer;
- malformed filesystem data;
- privilege escalation;
- memory corruption;
- malicious input/device data;
- interrupted update;
- information leakage through diagnostics.

Security requirements must map to concrete threats.

---

# PART XIV — ERROR UX, BSOD, RSOD, RECOVERY

# 72. ERROR ESCALATION

Preferred order:

```text
Application error
 ↓
Application restart
 ↓
Service recovery/restart
 ↓
Subsystem recovery
 ↓
Recovery environment
 ↓
Kernel/system stop
```

The OS should stop at the lowest level that guarantees safety.

---

# 73. NORMAL ERROR UX

Normal errors should explain:

- what happened;
- affected component;
- whether the system remains usable;
- what recovery was attempted;
- what the user can do;
- Error ID;
- technical details on request;
- support-report option where appropriate.

Normal failures must not unnecessarily look like system death.

---

# 74. ERROR ID MODEL

Conceptual stable ID:

`SB.<DOMAIN>.<CLASS>.<CODE>`

Exact syntax may be finalized later.

The ID is language-independent.

---

# 75. BSOD

Meaning:

> The OS was running, but the kernel determined that continuing is unsafe or impossible.

BSOD is reserved for serious system-level failures.

Possible visible information:

- stable error ID;
- failure class;
- exception/vector;
- CPU error code;
- instruction pointer;
- selected registers;
- process/thread;
- subsystem;
- kernel version/build;
- boot/session ID;
- crash-dump status;
- recovery/reboot status;
- report ID.

Ordinary users see a readable explanation. Advanced users can expand technical information.

---

# 76. RSOD

Meaning:

> The normal display/recovery path cannot itself be trusted, or critically important early-boot/integrity state has failed.

Use only for exceptional conditions such as:

- graphics/recovery path is itself broken;
- critical early boot state is corrupt;
- display/recovery initialization cannot safely continue;
- required integrity information is unavailable.

RSOD is not a routine crash screen.

---

# 77. CRASH DUMP

Possible contents:

- kernel state;
- thread/process metadata;
- fault address;
- registers;
- safe stack data;
- relevant event history;
- build/version information.

Full-memory dumps are sensitive and require explicit storage/access policy.

---

# 78. SUPPORT REPORT

The report may contain:

- OS version;
- kernel version;
- boot/session ID;
- hardware summary;
- driver summary;
- relevant logs;
- package/update state;
- storage health metadata;
- network state metadata;
- error IDs.

Exclude passwords, tokens, private keys and other sensitive credentials.

---

# 79. RECOVERY

Component-specific recovery should include as available:

- restart application;
- restart service;
- reset network service;
- restart display service when safe;
- rollback package transaction;
- restore configuration;
- filesystem check;
- repair boot metadata;
- recovery boot.

Recovery paths require loop protection and failure limits.

---

# PART XV — UPDATE SYSTEM

# 80. UPDATE LIFECYCLE

```text
DISCOVER
 ↓
VERIFY METADATA
 ↓
RESOLVE DEPENDENCIES
 ↓
DOWNLOAD
 ↓
VERIFY ARTIFACTS
 ↓
STAGE
 ↓
PRE-ACTIVATION CHECKS
 ↓
ACTIVATE TRANSACTIONALLY
 ↓
VERIFY
 ↓
COMMIT
```

## 80.1 Interrupted update

The system must be recoverable if power or reboot interrupts the update.

## 80.2 Rollback

Supported update classes require a known rollback method or a documented limitation.

## 80.3 Configuration migration

Persistent schemas need version identifiers and migration procedures.

---

# PART XVI — PERFORMANCE AND CLEAN SYSTEM

# 81. PERFORMANCE MODEL

Measure:

- boot duration;
- idle RAM;
- idle CPU;
- background wakeups;
- storage footprint;
- GUI latency;
- app startup;
- package download size;
- package installation time;
- update size/time.

Avoid “optimization” without measurement.

---

# 82. CLEAN SYSTEM POLICY

Always-on services need justification.

Optional features should be lazy-loaded where possible.

No duplicated caches without justification.

Generated/temporary data should have explicit ownership and cleanup rules.

---

# 83. DATA CLEANUP

Safe candidates include:

- expired temp data;
- obsolete package caches;
- abandoned incomplete downloads;
- provably orphaned generated files.

Never automatically delete:

- user documents;
- unknown data;
- active configuration;
- recovery material;
- security material.

---

# PART XVII — MINECRAFT AND GENERAL-PURPOSE DESIGN

# 84. GENERAL-PURPOSE CORE

SB must remain a general-purpose OS.

Minecraft-specific optimizations belong above the generic foundation wherever practical.

---

# 85. MINECRAFT / JVM LAYER

Potential components include:

- JVM runtime integration;
- instance manager;
- Minecraft launch management;
- Minecraft Server manager;
- performance profiles;
- storage/cache policies;
- workload-aware scheduling policy;
- benchmark tools.

These must not compromise the general-purpose kernel.

Architecture:

```text
SB Kernel
 ↓
Generic services
 ↓
SB Runtime / Policy
 ↓
JVM
 ↓
Minecraft tooling
```

Any performance claim must be benchmarked.

---

# 86. PERFORMANCE PROFILES

Future profiles may include:

- Balanced;
- Performance;
- Gaming;
- Power saving;
- Custom.

Profiles adjust supported policies without bypassing safety/security boundaries.

---

# PART XVIII — HARDWARE COMPATIBILITY AND RELEASE FAMILIES

# 87. FIRST TARGET

Initial stable target:

**generic x86_64 PC + QEMU**

---

# 88. COMPATIBILITY EXPANSION

After the common foundation is stable:

```text
Generic desktop/laptop
 ↓
Gaming PC
 ↓
Workstation
 ↓
High-end GPU systems
 ↓
Professional GPU systems
 ↓
Server/data-center class hardware
```

This is expansion of one architecture, not premature creation of separate forks.

---

# 89. RELEASE ARTIFACT STRATEGY

Eventually releases may provide different optimized profiles/artifacts if actual compatibility data justifies it:

- Generic;
- Gaming;
- Workstation;
- High-performance GPU;
- Professional/server-oriented.

Do not multiply ISO variants merely to create the appearance of hardware support.

---

# PART XIX — DEVELOPMENT AND REPOSITORY

# 90. TARGET REPOSITORY STRUCTURE

```text
.
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
│  ├─ security/
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

This is the target architecture, not an instruction to create empty directories.

---

# 91. DEVELOPMENT ENVIRONMENT

A reproducible browser-accessible development environment should eventually be supported with a development container/Codespaces-compatible configuration.

The environment should provide or document:

- C compiler;
- binutils/assembler;
- linker;
- ISO tools;
- GRUB tools where used;
- QEMU;
- test tools;
- source-control tooling.

The developer environment must be versioned with the repository where practical.

---

# 92. BUILD SYSTEM

Build must explicitly control:

- architecture;
- ABI;
- freestanding mode;
- compiler flags;
- linker flags;
- assembly mode;
- relocation assumptions;
- runtime helper dependencies;
- output artifacts.

The project has already encountered a class of error where 32-bit assembly attempted to express an invalid 64-bit relocation. Architecture assumptions must therefore be explicit in both source and CI.

---

# 93. CI

Minimum stages:

```text
checkout
 ↓
dependency setup
 ↓
static checks
 ↓
compile
 ↓
assemble
 ↓
link
 ↓
layout inspection
 ↓
ISO creation
 ↓
Multiboot validation
 ↓
QEMU boot
 ↓
serial log validation
 ↓
subsystem tests
 ↓
userspace tests
 ↓
GUI tests
 ↓
first-run tests
 ↓
persistence tests
 ↓
package tests
 ↓
recovery tests
 ↓
artifact verification
```

A test may not be weakened simply to make CI green.

---

# 94. QEMU MATRIX

Eventually cover:

- low-memory configuration;
- normal-memory configuration;
- multiple CPUs;
- different disk sizes;
- network disabled/enabled;
- framebuffer fallback;
- corrupted configuration;
- interrupted package transaction;
- interrupted update;
- recovery boot;
- controlled exception tests.

QEMU is a baseline environment, not proof of real-hardware compatibility.

---

# 95. REAL-HARDWARE VALIDATION

Real hardware tests must record:

- machine/platform;
- CPU;
- RAM;
- GPU;
- storage controller/device;
- network adapter;
- display setup;
- firmware mode;
- exact SB build;
- test results;
- limitations.

Hardware support claims must be backed by evidence.

---

# PART XX — IMPLEMENTATION ORDER FROM ZERO TO RELEASE

# 96. PHASE 0 — BUILD FOUNDATION

Deliver:

- reproducible build;
- assembly;
- linker;
- ELF;
- ISO;
- QEMU boot;
- serial logging;
- CI.

Acceptance:

- clean build works;
- ISO boots;
- kernel entry reached;
- CI reproducibly verifies it.

---

# 97. PHASE 1 — BOOT

Deliver:

- boot entry;
- CPU mode;
- stack;
- boot information validation;
- kernel image layout;
- protected boot ranges;
- early diagnostics.

---

# 98. PHASE 2 — MEMORY

Deliver:

- PMM;
- physical range reservation;
- VMM;
- kernel heap;
- memory self-tests.

Acceptance:

- allocations are safe;
- reserved memory is protected;
- VMM mapping works;
- heap allocate/free works;
- failures are observable.

---

# 99. PHASE 3 — CPU RUNTIME

Deliver:

- GDT;
- TSS;
- IDT;
- exceptions;
- interrupt controllers;
- timer abstraction;
- synchronization;
- ACPI groundwork;
- SMP groundwork.

---

# 100. PHASE 4 — EXECUTION

Deliver:

- scheduler;
- threads;
- processes;
- address spaces;
- IPC;
- syscall ABI;
- ELF loader;
- initial userspace;
- service manager.

---

# 101. PHASE 5 — STORAGE

Deliver:

- block abstraction;
- controller drivers;
- partition layer;
- filesystem;
- VFS;
- permissions;
- persistent configuration;
- installer storage support.

---

# 102. PHASE 6 — HARDWARE I/O

Deliver:

- PCI;
- ACPI integration;
- keyboard;
- mouse;
- USB foundations;
- display/framebuffer;
- storage devices;
- network devices;
- audio;
- power management.

---

# 103. PHASE 7 — GUI

Deliver:

- display service;
- text/font system;
- event system;
- GUI toolkit;
- compositor;
- window system;
- desktop shell;
- notifications;
- clipboard;
- File Manager.

---

# 104. PHASE 8 — FIRST BOOT / LOCALIZATION

Deliver:

- configuration service;
- locale service;
- language catalogs;
- keyboard layout;
- IME framework;
- first-run popup;
- persistence;
- Settings integration.

---

# 105. PHASE 9 — NETWORK

Deliver:

- Ethernet/link;
- IPv4;
- IPv6;
- ARP/ND;
- routing;
- UDP;
- TCP;
- DNS;
- DHCP;
- sockets;
- Network Manager;
- firewall/policy.

---

# 106. PHASE 10 — SOFTWARE DISTRIBUTION

Deliver:

- package metadata;
- package format;
- repository metadata;
- resolver;
- downloader;
- integrity/signature verification;
- transaction engine;
- package DB;
- CLI package tools;
- Store GUI;
- cache management.

---

# 107. PHASE 11 — SECURITY / RECOVERY

Deliver:

- accounts;
- permissions;
- security boundaries;
- secret storage;
- application isolation;
- signed updates;
- crash dump;
- support report;
- recovery environment;
- rollback.

---

# 108. PHASE 12 — PERFORMANCE / COMPATIBILITY

Deliver:

- benchmarks;
- boot optimization;
- memory optimization;
- scheduler tuning;
- GPU acceleration;
- driver expansion;
- hardware matrix;
- gaming/Minecraft profiles.

---

# 109. PHASE 13 — RELEASE

Deliver:

- release candidate;
- clean install verification;
- upgrade verification;
- recovery verification;
- hardware verification;
- localization verification;
- package verification;
- security review;
- artifact verification;
- public documentation;
- release publication.

---

# PART XXI — TEST SPECIFICATION

# 110. BOOT TESTS

Test:

- cold boot;
- warm reboot;
- shutdown;
- boot with expected memory map;
- boot with low memory;
- boot with altered device inventory;
- boot after interrupted previous session.

---

# 111. MEMORY TESTS

Test:

- first page allocation;
- repeated allocations;
- exhaustion;
- free/reallocate;
- invalid pointer/address;
- unaligned free;
- double free;
- reserved range protection;
- page-table ownership;
- heap fragmentation;
- allocator corruption response.

---

# 112. PROCESS/USERSPACE TESTS

Test:

- create process;
- create thread;
- schedule;
- exit;
- wait;
- invalid syscall pointer;
- invalid handle;
- malformed ELF;
- valid ELF;
- page fault in userspace;
- process cleanup after crash.

---

# 113. STORAGE TESTS

Test:

- read;
- write;
- create;
- rename;
- delete;
- directory enumeration;
- permission denial;
- interrupted configuration write;
- filesystem recovery behavior.

---

# 114. GUI TESTS

Test:

- display initialization;
- keyboard;
- mouse;
- event routing;
- window creation/destruction;
- focus;
- text rendering;
- long strings;
- missing translation fallback;
- notification;
- clipboard;
- accessibility settings.

---

# 115. FIRST-RUN TESTS

Test:

1. fresh configuration;
2. popup appears;
3. language list is selectable;
4. language change works;
5. keyboard layout is separately selectable;
6. configuration saves;
7. reboot preserves language;
8. corrupted configuration re-enters safe setup;
9. write failure produces useful error;
10. setup does not require terminal.

---

# 116. PACKAGE TESTS

Test:

- dependency resolution;
- conflict resolution;
- download failure;
- resume;
- checksum failure;
- signature failure;
- install;
- remove;
- update;
- rollback;
- GUI/CLI contention;
- interrupted transaction.

---

# 117. ERROR/RECOVERY TESTS

Test:

- recoverable application error;
- service failure/restart;
- network service restart;
- display service recovery where safe;
- controlled kernel exception;
- BSOD diagnostic collection;
- recovery boot;
- support report redaction;
- rollback after failed update.

---

# PART XXII — DATA, STATE, AND LIFECYCLE DETAILS

# 118. SESSION IDENTITY

Each boot/session should have a unique identifier sufficient to correlate logs, errors and support reports without requiring personally identifying data.

# 119. CONFIGURATION LIFECYCLE

```text
DEFAULT
 ↓
DISCOVERED/LOADED
 ↓
VALIDATED
 ↓
ACTIVE
 ↓
MODIFIED
 ↓
COMMITTED
```

Corrupt state:

```text
CORRUPT
 ↓
QUARANTINE/backup
 ↓
SAFE DEFAULT or RECOVERY
```

# 120. SERVICE LIFECYCLE

```text
DECLARED
 ↓
AVAILABLE
 ↓
STARTING
 ↓
RUNNING
 ↓
STOPPING
 ↓
STOPPED
```

Failed service:

```text
FAILED
 ↓
RETRY POLICY
 ↓
RESTARTING
 ↓
RUNNING or QUARANTINED
```

Restart loops must be bounded.

# 121. DEVICE LIFECYCLE

```text
DISCOVERED
 ↓
DRIVER_MATCHED
 ↓
ATTACHED
 ↓
ACTIVE
 ↓
QUIESCED
 ↓
DETACHED
```

Hotplug support must not leave stale resource ownership.

---

# PART XXIII — PUBLIC DOCUMENTATION AND ECOSYSTEM

# 122. WEBSITE

The official project website should eventually be hosted through a free/low-cost static service such as GitHub Pages.

It should explain:

- what SB is;
- why it exists;
- current status;
- downloads;
- supported hardware;
- installation;
- documentation;
- troubleshooting;
- development;
- security;
- release notes;
- community links.

Public claims must match verified implementation state.

# 123. COMMUNITY

Official public presence may include:

- GitHub;
- documentation site;
- community/Discord;
- official social account;
- release channels.

The operating-system implementation remains the primary technical source of truth.

---

# PART XXIV — AI DEVELOPMENT PROTOCOL

# 124. START-OF-TASK PROCEDURE

Before changing code, an AI must:

1. Read this document.
2. Inspect current repository state.
3. Identify relevant source files.
4. Inspect current tests/CI.
5. Determine the real current state.
6. Identify the earliest unverified prerequisite.
7. Make the smallest sound change.
8. Build.
9. Run targeted runtime tests.
10. Run relevant regression tests.
11. Inspect actual logs.
12. Update documentation if architecture changed.
13. State exactly what was and was not verified.

# 125. AI MUST NOT

- claim unimplemented features are complete;
- claim CI passed without checking;
- claim hardware support without hardware evidence;
- remove tests to hide failure;
- bypass architecture layers for convenience;
- create GUI mocks and call them functional;
- put first-run language setup into the terminal path;
- silently delete data;
- invent behavior for intentionally undecided architecture;
- create a duplicate subsystem with competing state ownership.

# 126. DEBUGGING LOOP

```text
REPRODUCE
 ↓
CAPTURE FACTS
 ↓
LOCALIZE FAILURE
 ↓
FORM ROOT-CAUSE HYPOTHESIS
 ↓
MAKE MINIMUM SOUND CHANGE
 ↓
BUILD
 ↓
TARGETED TEST
 ↓
REGRESSION TEST
 ↓
DOCUMENT
```

Do not make unrelated speculative changes just to “see what happens.”

---

# PART XXV — UNDECIDED TECHNICAL ITEMS

# 127. Why explicit TBD matters

Some implementation choices cannot responsibly be frozen before the relevant subsystem exists and is measured.

These are explicitly undecided until a documented architectural decision is made:

- final filesystem;
- final package archive format;
- final package repository metadata format;
- exact GUI/window-system IPC protocol;
- final compositor technology;
- final authentication backend;
- final sandbox mechanism;
- final secret-storage implementation;
- final accelerated-graphics API/backend;
- exact timer priority across all hardware classes;
- complete storage-controller coverage;
- complete Wi-Fi chipset coverage;
- server-oriented profile architecture.

When one is decided, record:

1. decision;
2. alternatives;
3. reason;
4. compatibility impact;
5. migration impact;
6. recovery implications;
7. implementation status.

Never fill TBD items with guesses simply to make the document look complete.

---

# PART XXVI — CURRENT PROJECT STATE

# 128. Current implementation reality

At the current stage the repository already contains an early x86_64 kernel, bootloader/build infrastructure, PCI enumeration, PMM/VMM/heap and several execution/runtime foundations plus numerous detailed design documents.

However, source existence is not equivalent to runtime verification.

The current critical path remains early kernel memory initialization.

High-level intended order from the present state:

```text
Boot / ISO / Multiboot
        ↓
PCI
        ↓
PMM  ← current critical verification area
        ↓
VMM
        ↓
Heap
        ↓
GDT/TSS/IDT/Interrupts
        ↓
Timer
        ↓
Scheduler
        ↓
Process/Syscall
        ↓
Userspace
        ↓
Storage
        ↓
Display/Input
        ↓
GUI
        ↓
First Boot
        ↓
Network
        ↓
Package/Store
        ↓
Security/Recovery/Updates
        ↓
Performance/Hardware expansion
        ↓
Release
```

The master document must be updated whenever this actual state changes materially.

---

# PART XXVII — FINAL DEFINITION OF SB DESKTOP

# 129. RELEASE-READY REQUIREMENTS

SB Desktop v1 is release-ready only when the target release can demonstrate all of the following.

## Foundation

- reproducible build;
- valid boot image;
- reliable boot;
- CPU foundation;
- physical/virtual memory;
- interrupts/timer;
- scheduler/processes;
- userspace.

## Storage

- supported storage works;
- filesystem works;
- VFS works;
- persistent configuration works;
- interrupted writes have safe behavior.

## Desktop

- display works;
- keyboard/mouse work;
- compositor/window system work;
- desktop shell works;
- applications can start/exit;
- settings work;
- terminal works.

## First boot

- graphical language popup works;
- four initial languages work;
- language persists;
- keyboard layout is independently configurable;
- first-run is recoverable.

## Network

- supported NIC works;
- network configuration works;
- DNS/sockets work where supported;
- GUI/CLI management works.

## Software

- packages can be searched;
- installed;
- removed;
- updated;
- verified;
- rolled back where supported;
- GUI and CLI transactions are coordinated.

## Diagnostics

- normal errors are understandable;
- serious failures have stable IDs;
- support reports exist;
- BSOD is implemented for genuine kernel emergencies;
- RSOD is reserved for defined extreme conditions;
- recovery is available for supported failures.

## Security

- actual privilege boundaries;
- validated syscalls;
- package/update trust;
- secret redaction;
- permission enforcement.

## Performance

- base system is measurably lightweight;
- optional features do not unnecessarily consume base resources;
- published performance claims are benchmark-backed.

## Compatibility

- supported hardware is documented;
- fallbacks exist where practical;
- unsupported hardware is identified honestly.

---

# 130. WHAT PERFECT SB MEANS

“Perfect” does not mean every imaginable feature is included.

It means the product consistently achieves:

```text
minimal unnecessary cost
+
strong performance
+
clarity
+
user choice
+
security
+
stability
+
recoverability
+
maintainability
+
extensibility
+
honest diagnostics
```

Every new feature must answer:

> Does this provide meaningful user or system value, and can it be provided without imposing unnecessary permanent cost on every user?

If yes, design it properly.

If no, keep it out of the base system or do not add it.

---

# 131. THE COMPLETE SB MENTAL MODEL

The finished project should be understandable through this model:

```text
                    SUIRABOX OS
                         │
       ┌─────────────────┴─────────────────┐
       │                                   │
   SMALL CORE                         USER CHOICE
       │                                   │
       ├─ Kernel                           ├─ Apps
       ├─ Memory                           ├─ Languages
       ├─ CPU/Interrupts                  ├─ Themes
       ├─ Scheduler                        ├─ Tools
       ├─ Storage                          ├─ Drivers
       ├─ Network                          └─ Optional Services
       ├─ Security
       └─ Recovery
                 │
                 ▼
             GUI DESKTOP
                 │
      ┌──────────┼──────────┐
      ▼          ▼          ▼
   Settings   Terminal    Store
      │          │          │
      └──────────┼──────────┘
                 ▼
          SHARED SYSTEM SERVICES
                 │
                 ▼
              HARDWARE
```

This is the central design: **a small reliable foundation, shared system services, a complete GUI, a complete CLI, and optional components that can be added without bloating every installation.**

---

# 132. PROJECT SURVIVAL GUARANTEE

This file exists to preserve project intent across:

- AI changes;
- developer changes;
- conversation loss;
- machine loss;
- development-environment migration;
- long periods of inactivity;
- refactoring;
- release branches;
- future hardware expansion.

If context is lost, the recovery procedure is:

```text
Read SB_OS_DESIGN.md
 ↓
Inspect main branch
 ↓
Inspect current commit
 ↓
Inspect CI
 ↓
Inspect actual implementation
 ↓
Locate first unverified prerequisite
 ↓
Continue from there
```

No original chat transcript is required to reconstruct the intended SB Desktop product when this file and the repository remain intact.

---

# 133. FINAL DEVELOPMENT COMMANDMENT

**Build from the foundation upward.**

**Verify every layer before depending on it.**

**Keep the base system small.**

**Let users choose what they need.**

**Keep the GUI complete and the terminal real.**

**Do not confuse plans, code and verified behavior.**

**Explain failures.**

**Protect user data.**

**Recover when possible.**

**Measure performance.**

**Expand hardware compatibility only after the common foundation is stable.**

**Never let the implementation silently drift away from the project described by this file.**

---

# 134. REFERENCE CHECKPOINTS

The implementation should consult authoritative sources for architecture/hardware details. Current examples include Intel's x86 architecture manuals, the UEFI Forum specifications, PCI-SIG specifications, and applicable USB/NVMe/ACPI documentation. These references are external technical authorities; their versions must be rechecked when a subsystem is implemented because specifications can change.

---

# END OF MASTER SPECIFICATION
