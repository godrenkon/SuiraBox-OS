# SuiraBox JVM Architecture

## Principle

The JVM is a userspace runtime. It is not part of the kernel.

```text
Minecraft / Java Applications
          |
          v
      SB JVM Runtime
          |
          v
     Userspace APIs
          |
          v
       SB Kernel
```

This keeps the kernel general-purpose while allowing SuiraBox to optimize Java workloads aggressively through documented policies and runtime integration.

## Runtime responsibilities

The SB runtime will manage:

- Java runtime discovery and selection
- JVM launch configuration
- heap sizing policy
- thread and CPU policy hints
- filesystem/cache locations
- diagnostics and crash information
- Minecraft instance integration

## Kernel responsibilities

The kernel provides:

- processes and isolated address spaces
- threads and scheduling
- virtual memory
- file and device I/O
- networking
- timers
- synchronization primitives
- security boundaries

The JVM must not directly manipulate hardware or kernel memory.

## Performance direction

JVM/Minecraft optimization will be evidence-driven. Candidate improvements include:

- workload-aware scheduling hints
- memory locality and page-size policy
- reduced I/O overhead
- asynchronous filesystem operations
- network batching where appropriate
- CPU topology-aware thread placement
- tuned default JVM flags for known Minecraft workloads

Each optimization needs a reproducible benchmark and a rollback path.

## Distribution model

The OS should make supported Java runtimes easy to install and switch between. The base OS does not need to bundle every JDK/JRE variant.

SB Hub can expose:

```text
Java Runtime
├── Installed runtimes
├── Recommended runtime
├── Per-instance runtime
└── Runtime diagnostics
```

## Minecraft integration

The Minecraft layer will sit above the generic JVM runtime:

```text
SB Hub
  |
  +-- Minecraft Launcher integration
  +-- Instance Manager
  +-- Mod Manager
  +-- Loader Manager
  +-- Server Manager
  |
  v
SB Java Runtime Manager
  |
  v
JVM
```

The same Java runtime infrastructure must remain usable by non-Minecraft Java applications.
