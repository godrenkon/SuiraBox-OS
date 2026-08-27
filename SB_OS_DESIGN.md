# SuiraBox OS — Master Design Specification

> Purpose: canonical implementation plan for AI-assisted development of SuiraBox OS (SB Desktop).
> Status: living specification. Code must follow this document unless a newer explicit design decision supersedes it.

## 1. Project Identity

- Project: **SuiraBox OS (SB OS)**
- Organization/community: **Suiram**
- Primary target: a lightweight, high-performance, user-configurable general-purpose **GUI desktop OS**.
- Source model: open source.
- Current architecture target: x86_64 PC.
- Immediate objective: finish one coherent, bootable GUI desktop OS before creating specialized editions.
- Do not implement or prioritize a CUI-only edition during the current project phase.

## 2. Core Philosophy

SB should be small by default and extensible on demand.

1. Keep the base OS as small as reasonably possible.
2. Do not ship optional software merely because it is conventional.
3. Put optional applications, language packs, drivers, services, themes, utilities, and other non-core components in an installable package system.
4. Prefer user choice over hard-coded behavior.
5. Settings should be detailed without becoming confusing.
6. Normal errors should be recoverable and understandable.
7. Kernel panic screens are exceptional diagnostic mechanisms, not normal error UX.
8. Performance, boot time, RAM use, disk use, network efficiency, and reliability are first-class requirements.
9. Avoid hidden, unnecessary, stale, duplicate, cache-like, or orphaned data where safe and technically justified.
10. Do not sacrifice correctness or security merely to remove a few bytes.
11. Command-line access remains a first-class capability inside the GUI edition; GUI must not replace a functional terminal.
12. Never claim hardware support or feature completion without a testable implementation.

## 3. Development Strategy

Build vertically from a stable foundation instead of creating disconnected mock features.

### Phase order

1. Boot and CPU initialization
2. Physical memory manager (PMM)
3. Virtual memory manager (VMM)
4. Kernel heap
5. GDT/TSS/IDT and exception handling
6. Interrupt controllers and timers
7. Scheduler and process/thread primitives
8. Syscalls and userspace boundary
9. Storage abstraction and filesystem support
10. Input and display/framebuffer infrastructure
11. GUI compositor/window system
12. Desktop shell
13. First-boot setup and settings persistence
14. Localization/i18n/l10n
15. Network stack and network configuration UI
16. Package manager and SB Store backend/client
17. Device-driver expansion
18. Recovery, diagnostics, logging, update system
19. Performance hardening and data cleanup
20. Release engineering and hardware compatibility expansion

A phase is not considered complete merely because its source exists. It must compile, boot/run where applicable, and have an appropriate automated or manual test.

## 4. Boot Architecture

Target boot path:

BIOS/UEFI or compatible boot environment
→ SB bootloader
→ 64-bit kernel entry
→ CPU/interrupt setup
→ memory initialization
→ kernel services
→ userspace initialization
→ display/input stack
→ GUI compositor
→ desktop
→ first-boot selector if required

The current development/debug terminal-like output is not the final user-facing first-boot experience.

### First boot

For the GUI edition, after the desktop-capable graphical environment is available, the first user-facing setup is a small GUI popup:

- Title: Welcome to SuiraBox
- Field: Language
- Control: select box/drop-down
- Initial languages:
  - 日本語
  - English
  - 中文
  - Español
- Button: Continue

After confirmation, persist the selection. On later boots, skip the popup and enter the desktop normally.

If configuration storage is missing or invalid, fall back safely to the setup/default path rather than looping forever.

Additional first-run settings may follow the language selection, but must remain lightweight. Candidate settings:

- Region/time zone
- Keyboard layout
- Network setup
- Privacy defaults
- Performance profile

Do not require terminal commands for normal GUI setup.

## 5. Localization

The localization architecture must be data-driven and shared by:

- desktop shell
- settings
- notifications
- error dialogs
- recovery UI
- installer/setup
- package/store UI
- terminal-facing human-readable messages where practical

Initial supported languages: Japanese, English, Chinese, Spanish.

The architecture must permit additional language packs later without requiring the entire base OS to contain every translation asset.

Language packs should be independently installable when practical.

## 6. Error and Crash UX

### Normal error hierarchy

`INFO → NOTICE → WARNING → ERROR → CRITICAL → RECOVERY`

Normal errors should explain:

- what happened
- affected component
- whether the OS is still safe to use
- what SB is doing automatically
- what the user can do next
- a stable Error ID
- a Details action
- a Support Report action when appropriate

Prefer component recovery over whole-system shutdown.

Example: a recoverable GPU-driver failure should attempt to restart the affected driver/session rather than immediately presenting a panic screen.

### BSOD

BSOD is a last-resort kernel diagnostic screen.

Meaning:

> The system was running, but the kernel determined that continuing was unsafe or impossible, so it stopped.

It should include, when available:

- error ID
- human-readable failure class
- exception/vector
- error code
- instruction pointer/register context
- process/thread
- kernel build/version
- boot/session ID
- relevant subsystem
- recovery result

### RSOD

RSOD is reserved for especially severe early-boot, display-output, recovery-path, or integrity failures where the normal graphical recovery path cannot be trusted.

Meaning:

> The normal display/recovery path itself is compromised, or required boot/system data cannot be trusted.

It must not be used for ordinary application errors.

### Support diagnostics

Diagnostic reports should be useful to both a technician and an advanced user while avoiding secrets. Never intentionally include passwords, authentication tokens, private keys, browser session credentials, or equivalent secrets.

## 7. Storage and Data Discipline

The OS must distinguish:

- immutable/base system data
- user data
- configuration
- package-managed data
- caches
- temporary data
- logs
- recovery data

Do not delete data solely because it appears unused if doing so can damage user data, package state, recovery, security, or diagnostics.

Implement cleanup as explicit, policy-driven maintenance with safe boundaries.

Configuration writes should be robust against interrupted writes. Prefer atomic replacement or journaled/transactional mechanisms where appropriate.

## 8. Package and SB Store Architecture

The base OS should not contain every optional feature.

Architecture:

`SB Core → Package Manager → Repository/Store → Optional Components`

Potential installable components:

- applications
- language packs
- fonts
- themes
- drivers/firmware packages where legally and technically appropriate
- desktop extensions
- development tools
- multimedia components
- network tools
- compatibility layers

Requirements:

- dependency resolution
- versioning
- integrity verification
- rollback/recovery
- uninstall without orphaning files where practical
- local cache management
- fast downloads through metadata minimization and parallel/resumable transfers where appropriate
- mirror/CDN support later
- signed repository metadata/packages for official releases

The package manager must remain usable from both GUI and terminal.

## 9. Network Architecture

Networking is a major subsystem, not an afterthought.

Required direction:

- Ethernet
- Wi-Fi through supported hardware/driver layers
- IPv4
- IPv6
- DNS
- DHCP
- static configuration
- routing
- firewall/security policy
- loopback
- sockets/API for userspace
- diagnostic tools

GUI must expose understandable network configuration while retaining full terminal tooling.

Network code must avoid blocking the desktop unnecessarily. Timeouts and failure states must be explicit.

## 10. Hardware Strategy

Initial compatibility target: broad x86_64 PC hardware, with graceful degradation on unsupported devices.

Do not tie the base OS to a single GPU vendor.

Future hardware expansion should cover, where feasible and legally supportable:

- mainstream integrated graphics
- AMD GPUs
- Intel GPUs
- NVIDIA GPUs including high-end RTX generations
- NVIDIA professional/data-center families
- high-core-count workstation/server CPUs
- modern storage controllers
- common USB and PCIe devices

GPU support must be layered so that a missing vendor driver does not prevent the base OS from booting into a usable fallback display mode.

Release artifacts may eventually be split by compatibility profile, but the first priority is one reliable general x86_64 GUI release.

## 11. GUI Architecture

The GUI must be modular.

Layers:

`Kernel/Drivers → Display/Input Services → Compositor → Window System → Desktop Shell → Applications`

Requirements:

- keyboard and mouse support
- accessibility foundation
- window management
- notifications
- settings application
- terminal application
- file management
- application launching
- crash/error dialogs
- recovery UI
- localization

The GUI must not require every optional application to be resident in memory at boot.

## 12. Settings Architecture

Settings must be granular but organized.

Suggested top-level categories:

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

Every setting should have a stable schema and clear ownership. Avoid duplicate sources of truth.

## 13. Terminal

The GUI edition must provide a real terminal and command execution environment.

Terminal functionality must not be treated as a cosmetic feature. System administration, development, diagnostics, scripting, package management, and recovery must have CLI equivalents where technically appropriate.

GUI and CLI should call the same underlying system APIs rather than maintaining incompatible duplicate implementations.

## 14. Security Model

Security must be layered:

- least privilege
- userspace/kernel separation
- memory protections where supported
- validated system-call boundaries
- package authenticity/integrity
- secure configuration handling
- permission model
- firewall/network controls
- safe update and rollback mechanisms
- sanitized diagnostic reporting

Do not implement security features merely as UI labels; each feature must have an enforcement point.

## 15. Reliability and Recovery

The OS should prefer recovery over panic.

Examples:

- restart failed userspace service
- restart display component when possible
- restart network service
- isolate a failed application
- preserve logs across reboot
- offer recovery mode
- support rollback for failed package/system updates

A panic is appropriate only when continuing would be unsafe or technically impossible.

## 16. Logging and Diagnostics

Use structured logs where practical.

Each significant system error should have a stable identifier and severity.

Diagnostic information should be searchable by:

- timestamp
- component
- severity
- error ID
- boot/session ID

Users should be able to create a support report from Settings/Recovery without needing a terminal.

## 17. Updates

Future update system requirements:

- signed metadata/packages
- dependency-aware updates
- transactional or rollback-capable system updates where feasible
- clear progress reporting
- offline/recovery path
- prevention of partial system states

Do not force optional packages into the base image merely to simplify updates.

## 18. Release Engineering

Every release should have a documented compatibility profile and test matrix.

Minimum release gates:

1. source builds reproducibly enough for the project target
2. Multiboot/boot image validation
3. QEMU boot smoke test
4. kernel panic/exception test coverage
5. memory-management tests
6. userspace boundary tests
7. storage tests
8. network tests
9. GUI startup test
10. first-boot setup test
11. language-selection persistence test
12. package install/remove test
13. recovery test
14. artifact integrity checks

Release artifacts should eventually include clear labels such as:

- Generic x86_64
- Hardware/driver profile where justified
- Debug/developer builds
- Stable builds

Do not multiply releases prematurely. Prefer one broadly compatible image until specialization is justified by real hardware/test data.

## 19. CI Rules

CI failures are engineering signals, not obstacles to bypass.

For a boot failure:

1. reproduce
2. identify exact phase
3. add temporary diagnostics if needed
4. fix root cause
5. remove unnecessary diagnostics
6. rerun the full relevant test path

Never mark a subsystem complete because the build succeeds if runtime tests still fail.

Avoid speculative chains of changes. Keep changes small enough to identify regressions.

## 20. Performance Rules

Performance goals include:

- minimal boot work
- lazy initialization where safe
- minimal resident base services
- efficient memory allocation
- efficient filesystem/network I/O
- no unnecessary polling
- no busy loops in normal operation
- bounded startup tasks
- minimal duplicate data
- low idle resource use

Optimization must be measured. Do not trade correctness for unmeasured micro-optimizations.

## 21. AI Development Rules

Any AI working on this repository must:

1. Read this document before making architectural changes.
2. Inspect existing code before inventing replacement architecture.
3. Preserve working behavior unless intentionally changing it.
4. Prefer root-cause fixes over symptom suppression.
5. Keep interfaces documented and stable.
6. Add tests for new critical behavior.
7. Never claim a feature is implemented without verifying the relevant code/build/test.
8. Clearly distinguish implemented, partially implemented, planned, and speculative features.
9. Do not silently remove functionality.
10. Do not add dependencies without justification.
11. Keep the base OS lightweight.
12. Treat security, data integrity, and recovery as requirements.
13. Keep the GUI edition as the sole current product target.
14. When blocked by missing infrastructure, document the blocker rather than fabricating completion.

## 22. Current Priority

At the present development stage, the priority is **boot stability and kernel foundations**.

Immediate sequence:

`PMM → VMM → Heap → Interrupts/Exceptions → Scheduler → Process/Syscall → Userspace → Display → GUI`

Do not move the project into broad application/store polish while the kernel cannot reliably pass the boot smoke test.

Once the GUI desktop is genuinely bootable, implement the first-boot language selector and persistent setup described above.

## 23. Definition of Done for the First Major Release

The first major SB Desktop release is not "done" until a normal user can:

1. boot the ISO on supported x86_64 hardware or QEMU
2. reach a graphical desktop without a terminal-based setup requirement
3. select a language on first graphical startup
4. reboot and retain that selection
5. configure network settings graphically
6. use a real terminal
7. install/remove optional software through the package system
8. change detailed system settings
9. receive understandable recoverable error messages
10. obtain useful diagnostic/support information after serious failures
11. recover from supported system/package failures
12. use the system without unnecessary optional components consuming base resources

The project should then expand compatibility and optimization from this stable foundation rather than replacing the foundation for every hardware category.
