# SuiraBox OS Scheduler

## Current stage

The scheduler currently provides a bootstrap task model and a timer-driven tick accounting boundary. It does not yet perform CPU context switching.

```text
PIT IRQ0
  |
  v
Timer IRQ stub
  |
  v
sb_timer_tick()
  |
  v
scheduler_tick()
  |
  v
Current task accounting
```

## Planned scheduler

1. Kernel threads
2. Saved CPU context
3. Ready queue
4. Blocking and wakeup
5. Preemption
6. Per-CPU run queues
7. Load balancing
8. Priority / class policies
9. Workload-aware policies for Minecraft and server workloads

## Minecraft direction

Minecraft-specific scheduling must remain a policy layer. The kernel scheduler should provide generic primitives such as priorities, CPU affinity, sleep/wakeup, and accounting. SB services can then choose policies appropriate to Minecraft, JVM, browser, or server workloads.

Performance claims must be established with repeatable benchmarks, including scheduler overhead, context-switch cost, latency distribution, and application throughput.
