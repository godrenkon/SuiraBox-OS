# SuiraBox OS Scheduler

## Current stage

The scheduler now has a verified timer-driven preemption path on x86_64 plus tested BLOCKED / SLEEPING transitions, timer wakeups, cross-process address-space switching, process exit/reaping, and parent/child wait-resume behavior. Userspace execution remains scheduler-owned rather than using a one-way direct ring3 transition.

The validated preemption path is:

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

The validated blocking process path is:

```text
parent PID 1 / ring3
    |
    | SPAWN trusted child image
    v
child PID 2 created
    |
    | WAIT(2)
    v
parent task -> BLOCKED
    |
    | scheduler selects child
    | CR3 + TSS.rsp0 switch
    v
child PID 2 / ring3
    |
    | EXIT(0)
    v
child process -> EXITED
    |
    | reap address space + kernel stack
    | wake registered parent waiter
    v
parent task -> READY
    |
    | scheduler restores saved int 0x80 frame
    v
WAIT returns 0 in parent
```

The independent sleep path is:

```text
user task RUNNING
    |
    | SLEEP(deadline)
    v
SLEEPING + descheduled
    |
    | timer ticks reach deadline
    v
READY
    |
    | later scheduler selection
    v
saved syscall frame resumes
```

GitHub Actions QEMU smoke testing verifies all three paths on the CPU, rather than only checking scheduler bookkeeping.

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

### Blocking syscall context

The `int 0x80` entry saves the same complete GPR image used by preemption. A syscall that blocks can therefore retain its exact saved frame, hand control back to the scheduler, and later resume at the instruction following `int 0x80` with its return value already installed in the saved `rax` slot.

This is used by both WAIT and SLEEP. It avoids creating a second, incompatible blocking-context representation for userspace syscalls.

### Cooperative kernel context

`sb_task_context_t` remains a smaller callee-saved context for explicit cooperative kernel switches through `sb_context_switch()`. It is intentionally not reused for timer preemption. Mixing the two models would lose registers when an IRQ arrives at an arbitrary instruction.

## Task states

The tested scheduler state machine now includes:

```text
READY -> RUNNING
RUNNING -> READY      timer preemption / reschedule
RUNNING -> BLOCKED    process wait
BLOCKED -> READY      child exit/reap completion
RUNNING -> SLEEPING   sleep syscall
SLEEPING -> READY     timer deadline reached
```

A BLOCKED or SLEEPING task is excluded from runnable selection. Wakeup changes it back to READY; actual execution resumes only when the scheduler later selects it.

The fixed task table is still scanned directly. The state semantics are working, but this is not yet the final run-queue data structure.

## Task selection and commit

Selection and state mutation are separate operations.

`scheduler_pick_next()` only finds a runnable task. It does not change `current_index`, mark the current task READY, or mark the candidate RUNNING.

A real switch path performs the commit only after a valid next execution context is available. This avoids the previous failure mode where selecting task B changed scheduler bookkeeping before the CPU had actually left task A.

## Address spaces and kernel stacks

Each scheduler task may carry:

- a CR3 value for its address space,
- a kernel stack base/top,
- a saved IRQ/syscall-frame pointer,
- a process ID,
- a dispatch counter,
- a user/kernel task marker,
- a wake deadline / blocking state as required by the transition.

Before returning to a selected task, scheduling switches CR3 when necessary and updates TSS.rsp0 to the selected task's kernel stack. QEMU now verifies an actual PID 1 -> PID 2 cross-process CR3 transition and later restores PID 1's address space.

Exited process resources are reclaimed only after the scheduler is no longer executing on that task's kernel stack/address space. The tested child exit path releases the process-owned user page tables/pages and scheduler-owned kernel stack before completing the waiting parent.

The current user-thread kernel stack policy is still intentionally small. Guard pages, larger configurable stacks, overflow diagnostics, and a general per-thread stack allocator policy remain future work.

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
9. A task in BLOCKED or SLEEPING state is never selected as runnable.
10. An exited task's executing kernel stack is not freed until scheduling has moved execution elsewhere.

## Verified milestone

The following roadmap items are considered verified by build + QEMU smoke tests:

- 3-6: timer to scheduler preemption connection
- 4-6: process-to-process CR3 switching
- 5-3: timer preemption path
- 5-4: resume from a saved register frame
- 5-5: READY / RUNNING / BLOCKED / SLEEPING transitions used by current runtime
- 5-6: timer sleep/wake integration
- 6-2: process object and address-space association
- 6-3: first user thread launch path
- 6-5: process exit / cleanup / reap path
- 6-6: parent-child wait lifecycle

The smoke test requires the following independent runtime sequences to occur.

### Preemption / saved-frame resume

```text
Scheduler: timer preemption switched to user task
Userspace: entering ring3
Userspace: first syscall reached kernel
Scheduler: user task preempted back to kernel
Scheduler: saved user IRQ frame selected for resume
Userspace: resumed user thread reached syscall
```

### Wait / child / reap

```text
Userspace: spawn syscall created child process
Scheduler: user task entered BLOCKED
Userspace: wait syscall blocked for child
Scheduler: cross-process CR3 switch verified
Scheduler: blocked user task descheduled
Userspace: child process PID syscall OK
Userspace: child requested process exit
Scheduler: blocked user task woke
Process: exited process resources reaped
Scheduler: woke blocked user task selected for resume
Userspace: parent wait resumed after child exit
```

### Sleep / wake

```text
Scheduler: user task entered SLEEPING
Userspace: sleep syscall requested
Scheduler: sleeping user task descheduled
Scheduler: sleeping user task woke
Scheduler: woke user task selected for resume
Userspace: woke sleeping user thread reached syscall
```

## Current limitations / next work

The scheduler is not yet a complete production scheduler. Remaining near-term work includes:

1. Replace fixed task-table scanning with explicit run-queue data structures.
2. Generalize spawn beyond the trusted boot-image selector and support ordinary process launch inputs.
3. Add general multi-thread-per-process execution and thread-level exit/join semantics.
4. Complete timeout semantics beyond the current sleep deadline and child wait cases.
5. Add guard pages and a final user/kernel stack policy.
6. Formalize CPU accounting and scheduler statistics.
7. Add priorities and affinity semantics.
8. Add SMP/per-CPU run queues and load balancing.
9. Add FPU/SIMD context ownership before allowing user/kernel code that relies on those facilities.
10. Benchmark context-switch overhead, wakeup latency, blocking latency, and scheduling fairness.

## Minecraft direction

Minecraft-specific scheduling remains a policy layer. The generic kernel scheduler should provide primitives such as priorities, CPU affinity, sleep/wakeup, accounting, and eventually scheduling classes. Higher layers can then select policies for Minecraft clients, JVM GC/compiler threads, Minecraft servers, browsers, or ordinary desktop workloads without hard-coding Minecraft behavior into the kernel's correctness-critical scheduler core.

Performance claims must be established with repeatable benchmarks, including scheduler overhead, context-switch cost, wakeup latency distribution, tail latency, and application throughput.
