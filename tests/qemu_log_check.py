#!/usr/bin/env python3
from pathlib import Path
import sys


def fail(message: str) -> None:
    raise SystemExit(message)


def require_all(log: str, markers: list[str]) -> None:
    for marker in markers:
        if marker not in log:
            fail(f"missing QEMU marker: {marker}")


def require_ordered(log: str, name: str, markers: list[str]) -> None:
    cursor = -1
    for marker in markers:
        position = log.find(marker, cursor + 1)
        if position < 0:
            fail(f"missing {name} marker after offset {cursor}: {marker}")
        cursor = position
    print(f"ordered QEMU proof OK: {name}")


def main() -> None:
    path = Path(sys.argv[1] if len(sys.argv) > 1 else "qemu.log")
    log = path.read_text(errors="replace")

    if "Exception:" in log:
        fail("unexpected kernel/user exception detected")

    require_all(
        log,
        [
            "Kernel initialized.",
            "PCI: scanning bus/device/function space...",
            "PCI: scan complete.",
            "Storage: ATA primary master registered",
            "Storage: FAT32 system mount /disk ready",
            "Memory: PMM bootstrap free pages =",
            "Memory: VMM map/translate/unmap OK",
            "Memory: kernel heap alloc/free OK",
            "CPU: GDT/TSS ready",
            "Scheduler: selection/commit separation OK",
            "Process/Syscall: model and dispatch OK",
            "Syscall: int 0x80 user gate ready",
            "Userspace: ELF + address-space + stack preparation OK",
            "Userspace: first user thread scheduled",
            "Timer: IRQ0 enabled",
            "Phase 1 bootstrap complete.",
            "Syscall: ABI v1 userspace probe OK",
            "Userspace: validated user pointer copy OK",
            "Syscall: invalid user pointer rejected",
            "Syscall: writable user pointer copy OK",
            "Syscall: read-only user pointer write rejected",
            "Handle: userspace process handle opened",
            "Handle: type and rights query OK",
            "Handle: close invalidated generation",
            "Handle: stale generation rejected",
            "VFS: system /boot module path open OK",
            "File: boot module opened as FILE handle",
            "File: invalid output pointer rejected before read",
            "File: FILE handle read copied to userspace",
            "File: FILE handle seek OK",
            "Userspace: FILE handle read/seek lifecycle OK",
            "File: generic VFS path opened as FILE handle",
            "Userspace: generic VFS FILE_OPEN lifecycle OK",
            "Userspace: runtime FAT32 file read through generic VFS OK",
            "Directory: generic VFS path opened as DIRECTORY handle",
            "Directory: entry copied to userspace",
            "Directory: end of directory reported",
            "Userspace: runtime FAT32 directory enumeration OK",
            "Pipe: userspace pipe handles created",
            "Pipe: data written from userspace",
            "Pipe: data read to userspace",
            "Pipe: EOF observed after writer close",
            "Userspace: PIPE handle lifecycle OK",
            "Event: userspace manual-reset event created",
            "Event: unsignaled wait blocked with timeout",
            "Event: SIGNAL woke blocked user task",
            "Userspace: cross-thread EVENT wake OK",
            "Userspace: EVENT wait/timeout lifecycle OK",
            "MessageQueue: userspace queue handle created",
            "MessageQueue: message enqueued from userspace",
            "MessageQueue: short receive buffer preserved message",
            "MessageQueue: message copied to userspace",
            "SharedMemory: userspace object handle created",
            "SharedMemory: writable mapping installed",
            "SharedMemory: read-only alias installed",
            "SharedMemory: mapping released",
            "Userspace: shared memory alias/permission lifecycle OK",
            "Service: named local endpoint registered",
            "Service: client connected by name",
            "Service: message sent through local transport",
            "Service: message received through local transport",
            "Userspace: child local service round-trip OK",
            "Userspace: cross-process local service round-trip OK",
            "Spawn: VFS path child wait completed",
            "Userspace: concurrent child processes armed",
            "Userspace: concurrent child processes completed",
            "Userspace: woke sleeping user thread reached syscall",
        ],
    )

    # The framebuffer is optional in headless CI; require one explicit outcome.
    if (
        "Display: framebuffer ready" not in log
        and "Display: framebuffer unavailable; using fallback console" not in log
    ):
        fail("missing display initialization outcome")

    sequences = {
        "syscall-abi/user-pointer": [
            "Userspace: first syscall reached kernel",
            "Syscall: ABI v1 userspace probe OK",
            "Userspace: validated user pointer copy OK",
            "Syscall: invalid user pointer rejected",
            "Syscall: writable user pointer copy OK",
            "Syscall: read-only user pointer write rejected",
        ],
        "handle-generation": [
            "Handle: userspace process handle opened",
            "Handle: type and rights query OK",
            "Handle: close invalidated generation",
            "Handle: stale generation rejected",
        ],
        "runtime-fat32-vfs": [
            "Storage: ATA primary master registered",
            "Storage: FAT32 system mount /disk ready",
            "Userspace: runtime FAT32 file read through generic VFS OK",
            "Directory: generic VFS path opened as DIRECTORY handle",
            "Directory: entry copied to userspace",
            "Directory: end of directory reported",
            "Userspace: runtime FAT32 directory enumeration OK",
        ],
        "pipe-handle-lifecycle": [
            "Pipe: userspace pipe handles created",
            "Pipe: data written from userspace",
            "Pipe: data read to userspace",
            "Pipe: EOF observed after writer close",
            "Userspace: PIPE handle lifecycle OK",
        ],
        "event-timeout-lifecycle": [
            "Event: userspace manual-reset event created",
            "Scheduler: timed user task entered BLOCKED",
            "Event: unsignaled wait blocked with timeout",
            "Scheduler: timed blocked task woke",
        ],
        "cross-thread-event-wake": [
            "Thread: additional user thread created",
            "Thread: THREAD_CREATE syscall registered user thread",
            "Event: blocking wait registered",
            "Event: SIGNAL woke blocked user task",
            "Event: manual-reset event signaled",
            "Userspace: secondary thread signaled event",
            "Userspace: cross-thread EVENT wake OK",
            "Event: signaled wait completed immediately",
            "Event: manual-reset event reset",
            "Userspace: EVENT wait/timeout lifecycle OK",
        ],
        "legacy-wait/child/reap": [
            "Spawn: executable staged from VFS file",
            "Userspace: spawn syscall created child process",
            "Scheduler: user task entered BLOCKED",
            "Userspace: wait syscall blocked for child",
            "Scheduler: blocked user task descheduled",
            "Scheduler: cross-process CR3 switch verified",
            "Userspace: child process PID syscall OK",
            "Userspace: child requested process exit",
            "Scheduler: blocked user task woke",
            "Process: exited process resources reaped",
            "Scheduler: woke blocked user task selected for resume",
        ],
        "general-spawn/service/wait/reap": [
            "Spawn: userspace request copied and validated",
            "Spawn: general child process created",
            "Spawn: general child PID syscall OK",
            "Service: client connected by name",
            "Service: message sent through local transport",
            "Service: message received through local transport",
            "Spawn: parent blocked waiting for general child",
            "Userspace: wait syscall blocked for child",
            "Userspace: child local service round-trip OK",
            "Spawn: general child requested process exit",
            "Process: exited process resources reaped",
            "Spawn: general child resources reaped",
            "Spawn: general child wait completed",
            "Userspace: cross-process local service round-trip OK",
        ],
        "message-queue": [
            "MessageQueue: userspace queue handle created",
            "MessageQueue: message enqueued from userspace",
            "MessageQueue: short receive buffer preserved message",
            "MessageQueue: message copied to userspace",
        ],
        "shared-memory": [
            "SharedMemory: userspace object handle created",
            "SharedMemory: writable mapping installed",
            "SharedMemory: read-only alias installed",
            "SharedMemory: mapping released",
            "Userspace: shared memory alias/permission lifecycle OK",
        ],
        "vfs-path-spawn/wait/reap": [
            "Spawn: VFS path request copied and validated",
            "Spawn: VFS path child process created",
            "Spawn: parent blocked waiting for VFS path child",
            "Userspace: wait syscall blocked for child",
            "Spawn: VFS path child PID syscall OK",
            "Spawn: VFS path child requested process exit",
            "Spawn: VFS path child resources reaped",
            "Spawn: VFS path child wait completed",
        ],
        # Scheduler sleep markers are global one-shot diagnostics. Keep their
        # proof separate from the later concurrent-process scenario.
        "scheduler-sleep/wake": [
            "Scheduler: user task entered SLEEPING",
            "Userspace: sleep syscall requested",
            "Scheduler: sleeping user task descheduled",
            "Scheduler: sleeping user task woke",
            "Scheduler: woke user task selected for resume",
        ],
        # This sequence is deliberately userspace-scoped. Nested service
        # rendezvous may consume the scheduler's one-shot sleep diagnostics
        # before this scenario begins.
        "concurrent-processes": [
            "Userspace: concurrent child processes armed",
            "Userspace: sleep syscall requested",
            "Userspace: woke sleeping user thread reached syscall",
            "Userspace: concurrent child processes completed",
        ],
    }

    for name, markers in sequences.items():
        require_ordered(log, name, markers)


if __name__ == "__main__":
    main()
