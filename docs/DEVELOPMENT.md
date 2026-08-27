# SuiraBox OS Development Guide

## Development environment

SuiraBox is developed to work without requiring a local PC during the early project stages.

Primary environment:

- GitHub repository
- GitHub Actions for reproducible CI builds and QEMU smoke tests
- Browser-based development environments such as GitHub Codespaces when available
- QEMU for x86_64 virtual hardware

## Rules for kernel changes

1. Keep changes small and testable.
2. Do not add a subsystem without a minimal observable test when practical.
3. Do not assume a performance optimization is beneficial without a benchmark.
4. Avoid vendor-specific assumptions in generic kernel interfaces.
5. Keep public ABIs and internal interfaces explicitly documented.
6. Treat security and reliability regressions as blockers for stable releases.

## Commit style

Use clear, scoped commit messages such as:

```text
boot: initialize long mode entry
kernel: add timer abstraction
memory: add physical page allocator
net: add IPv4 packet parsing
```

## Branching

The initial project may use `main` for early bootstrap commits. As the contributor base grows, feature branches and pull requests should become the normal workflow.

## Validation

Every kernel change should aim to pass:

- compiler warnings as errors;
- image validation;
- QEMU boot smoke test;
- relevant subsystem tests;
- benchmark/regression checks when performance-sensitive code changes.

## Public project quality

Before a stable release, the repository should also contain:

- LICENSE
- CONTRIBUTING.md
- CODE_OF_CONDUCT.md
- SECURITY.md
- SUPPORT.md
- release/changelog policy
- third-party notices
- supported hardware/platform matrix
- reproducible build instructions
- benchmark methodology

Minecraft names, logos, assets, and third-party components must be handled according to their respective terms. SuiraBox must not imply endorsement or affiliation where none exists.
