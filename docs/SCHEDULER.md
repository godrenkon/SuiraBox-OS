# SuiraBox OS Scheduler

## Current stage

The scheduler now has a verified timer-driven preemption path on x86_64. The first userspace thread is owned by the scheduler rather than launched through a one-way direct ring3 transition.

The validated runtime path is:

```text
PIT IRQ0
  |
  v
x86_64 IRQ entry
  |  save all GPRs
  v
sb_timer_tick(current_frame)
  |
  +--> scheduler_tick()
  |
  v
scheduler_preempt(current_frame)
  |  save current IRQ-frame pointer
  |  select runnable task
  |  commit task state/current task
  |  switch CR3 when required
  |  update TSS.rsp0
  v
next saved/synthetic IRQ frame
  |
  v
x86_64 IRQ epilogue
  |  restore all GPRs
  v
iretq
```

GitHub Actions QEMU smoke testing verifies the full round trip:

```text
kernel bootstrap task
    |
    | timer preemption
    v
first user task / ring3
    |
    | int 0x80 syscall
    v
kernel syscall path
    |
    | timer preemption
    v
kernel bootstrap task
    |
    | later timer preemption
    v
saved user IRQ frame
    |
    | iretq
    v
resumed user task
    |
    | int 0x80 syscall
    v
kernel
```

The second user syscall occurs only after the user task has been dispatched from its previously saved IRQ frame, so the smoke test validates actual CPU execution after resume rather than merely validating scheduler bookkeeping.

## Context models

SuiraBox currently keeps two context-switch mechanisms separate because they have different ABI requirements.

### IRQ-driven preemption context

`sb_irq_frame_t` represents an asynchronously interrupted CPU state. The IRQ entry saves every general-purpose register because an interrupt may occur at any instruction; caller-saved/callee-saved assumptions from the SysV function-call ABI do not apply.

The software-saved register order is:

```text
r15 r14 r13 r12 r11 r10 r9 r8
rdi rsi rbp rdx rcx rbx rax
```

It is followed by the hardware interrupt-return frame:

```text
rip cs rflags [rsp ss]
```

`rsp` and `ss` are physically pushed by the CPU only when the interrupt crosses privilege levels. Scheduler code must therefore never read those fields from a same-CPL kernel IRQ frame. Synthetic first-user-thread frames contain the full `rip/cs/rflags/rsp/ss` return state because their first `iretq` enters ring3.

### Cooperative kernel context

`sb_task_context_t` remains a smaller callee-saved context for explicit cooperative kernel switches through `sb_context_switch()`. It is intentionally not reused for timer preemption. Mixing the two models would lose registers when an IRQ arrives at an arbitrary instruction.

## Task selection and commit

Selection and state mutation are separate operations.

`scheduler_pick_next()` only finds a runnable task. It does not change `current_index`, mark the current task READY, or mark the candidate RUNNING.

A real switch path performs the commit only after a valid next execution context is available. This avoids the previous failure mode where selecting task B changed scheduler bookkeeping before the CPU had actually left task A.

## Address spaces and kernel stacks

Each scheduler task may carry:

- a CR3 value for its address space,
- a kernel stack base/top,
- a saved IRQ-frame pointer,
- a process ID,
- a dispatch counter,
- a user/kernel task marker.

Before returning to a selected task, preemption switches CR3 when necessary and updates TSS.rsp0 to the selected task's kernel stack. The current address-space implementation keeps the kernel mapping reachable across the tested bootstrap/user address spaces, which allows scheduler/IRQ code to complete safely while changing CR3.

The current first-user-thread kernel stack is one physical page. This is sufficient for the present minimal interrupt/syscall path but is not the final stack policy. Guard pages, larger configurable stacks, overflow diagnostics, and cleanup on thread exit remain future work.

## Interrupt invariants

The current x86_64 timer path relies on these invariants:

1. IRQ0 uses an interrupt gate, so IF is cleared on entry and the preemption path is not recursively interrupted by another maskable IRQ before `iretq`.
2. All GPRs are saved before entering C.
3. DF is cleared before calling C code.
4. The stack is aligned for the SysV AMD64 C ABI before `sb_timer_tick()` is called.
5. PIC IRQ0 is acknowledged only after the interrupted register state has been captured.
6. The IRQ epilogue restores the frame selected by the scheduler, not necessarily the frame that entered the handler.
7. TSS.rsp0 belongs to the task that will execute next when that task can enter ring0 from ring3.
8. A CR3 transition is performed before returning to the selected task when its address space differs from the current one.

## Verified milestone

The following roadmap items are considered verified by build + QEMU smoke tests:

- 3-6: timer to scheduler preemption connection
- 5-3: timer preemption path
- 5-4: resume from a saved register frame
- 6-3: first user thread launch path

The test requires all of these runtime markers in sequence to have occurred during the boot:

```text
Scheduler: timer preemption switched to user task
Userspace: entering ring3
Userspace: first syscall reached kernel
Scheduler: user task preempted back to kernel
Scheduler: saved user IRQ frame selected for resume
Userspace: resumed user thread reached syscall
```

## Current limitations / next work

The scheduler is not yet a complete production scheduler. Remaining near-term work includes:

1. Complete READY / RUNNING / BLOCKED / SLEEPING transitions.
2. Integrate sleep deadlines and wakeups with timer scheduling.
3. Define proper run-queue data structures instead of scanning the fixed task table.
4. Add thread exit and resource cleanup semantics.
5. Add multiple user threads/processes and validate process-to-process CR3 switching.
6. Formalize CPU accounting and scheduler statistics.
7. Add priorities and affinity semantics.
8. Add SMP/per-CPU run queues and load balancing.
9. Add FPU/SIMD context ownership before allowing user/kernel code that relies on those facilities.
10. Benchmark context-switch overhead and scheduling latency.

## Minecraft direction

Minecraft-specific scheduling remains a policy layer. The generic kernel scheduler should provide primitives such as priorities, CPU affinity, sleep/wakeup, accounting, and eventually scheduling classes. Higher layers can then select policies for Minecraft clients, JVM GC/compiler threads, Minecraft servers, browsers, or ordinary desktop workloads without hard-coding Minecraft behavior into the kernel's correctness-critical scheduler core.

Performance claims must be established with repeatable benchmarks, including scheduler overhead, context-switch cost, wakeup latency distribution, tail latency, and application throughput.
