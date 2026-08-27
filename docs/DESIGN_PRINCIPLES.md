# SuiraBox Design Principles

## 1. Minimal by default

The base installation must contain only components that are necessary for boot, security, recovery, hardware discovery, storage, networking, and the chosen user profile.

Optional services must not consume CPU time, memory, storage, or network bandwidth when disabled.

## 2. User choice first

Users should be able to add, remove, enable, disable, replace, and reconfigure major components without reinstalling the operating system.

A feature being built into SuiraBox does not mean that it must be enabled by default.

## 3. One core, many optimized editions

All editions share a common kernel/runtime ABI where practical, while profiles select specialized policies:

- Desktop
- Minecraft
- Gaming
- Workstation
- Server
- DataCenter

Hardware backends and policy modules must remain replaceable so future NVIDIA, AMD, Intel, and other hardware support can be added without redesigning the whole operating system.

## 4. Performance must be measurable

Optimization decisions should be backed by repeatable benchmarks and telemetry that users can disable.

Priority order:

1. Correctness and data integrity
2. Predictable latency
3. Resource efficiency
4. Throughput
5. Convenience

No optimization should silently trade away correctness.

## 5. Fast boot without fake progress

Startup should perform independent initialization in parallel where safe. The user interface may show useful onboarding or status information while background initialization continues, but progress reporting must remain truthful.

## 6. Modular software management

Core components, drivers, runtimes, applications, Minecraft instances, mod loaders, and server profiles should be independently installable and removable when technically safe.

The package and service model should make dependencies explicit so removing one component does not unexpectedly break unrelated applications.

## 7. Minecraft-first, not Minecraft-only

SuiraBox Minecraft Edition optimizes the JVM, graphics path, storage, networking, scheduling, and process policies for Minecraft workloads. The base platform remains a general-purpose operating system.

## 8. Open source and reproducible development

Builds, tests, release artifacts, compatibility information, and major design decisions should be public. Hardware-specific optimizations must be isolated so unsupported devices do not prevent the generic system from building.

## 9. Release by capability

Releases may be split by hardware and workload once the common core is stable. Examples include generic x86_64, NVIDIA-focused, AMD-focused, server, and DataCenter variants.

The first complete release should remain a single reference platform. Derivatives come after the reference platform is stable.
