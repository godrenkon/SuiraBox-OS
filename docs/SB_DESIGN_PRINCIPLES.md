# SuiraBox Design Principles

SuiraBox is designed around four priorities: minimal base system, user choice, high performance, and a UI that does not require terminal knowledge.

## 1. Minimal Core

The base installation should contain only components required to boot, recover, update, manage storage/network, install software, and provide a usable desktop foundation.

Optional applications, language runtimes, development kits, games, server stacks, large media components, and specialized drivers should remain outside the immutable/minimal core whenever practical.

## 2. Everything Optional Should Be Removable

Optional components must have explicit ownership and dependency metadata so the user can remove them without leaving unrelated services, files, startup tasks, or configuration behind.

When a component is shared by multiple installed applications, the package manager must explain the dependency before removal rather than silently deleting shared functionality.

## 3. GUI First, CLI Available

Every common package-management operation should be possible from the graphical SB Store/UI.

The terminal and `sb` command line remain first-class interfaces for power users, automation, recovery, and servers. The GUI is not a wrapper that requires command-line knowledge.

## 4. Fast Downloads

The package system should prefer small manifests, compressed/delta artifacts, parallel downloads where safe, resumable transfers, local caching, and geographically appropriate mirrors/CDN endpoints.

The UI should begin installing a selected component as soon as dependency metadata is resolved instead of downloading unnecessary catalog data.

## 5. Settings Without Complexity

Settings are grouped by user goal rather than by internal subsystem. Advanced settings remain available, but common settings should be understandable without knowing kernel terminology.

Examples:

- Performance
- Gaming
- Minecraft
- Network
- Storage
- Graphics
- Privacy & Security
- Updates
- Power
- Appearance
- Accessibility
- Developer
- Server

A search function must reach every setting.

## 6. No Silent Resource Waste

A service should not remain running merely because software was installed in the past. Components should declare whether they are boot-critical, on-demand, background, or user-started.

The system should expose startup/background activity so users can disable optional activity without editing configuration files manually.

## 7. Safe Defaults, Real Choice

Security-critical defaults remain safe. Optional behavior that is not security-critical should generally be configurable.

The OS should explain the effect, resource impact, and recovery path before applying potentially disruptive changes.

## 8. Profiles Are Packages of Choices

Desktop, Minecraft, Gaming, Workstation, Server, and DataCenter editions are profile selections over a shared core rather than unrelated operating systems.

A profile can enable different packages, services, drivers, scheduling policies, and default settings without forking the kernel unnecessarily.

## 9. Recovery Is a Core Feature

Package installation, updates, drivers, and system configuration should be transaction-like where practical. Failed operations should be reversible, and the recovery environment should be able to remove a broken optional component.

## 10. Open by Design

Repository metadata formats, package manifests, settings schemas, and profile definitions should be documented and machine-readable so community repositories and third-party tools can integrate without reverse engineering the implementation.
