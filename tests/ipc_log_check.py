#!/usr/bin/env python3
from pathlib import Path
import sys


def fail(message: str) -> None:
    raise SystemExit(message)


def require_ordered(log: str, name: str, markers: list[str]) -> None:
    cursor = -1
    for marker in markers:
        position = log.find(marker, cursor + 1)
        if position < 0:
            fail(f"missing ordered {name} marker after offset {cursor}: {marker}")
        cursor = position
    print(f"ordered IPC proof OK: {name}")


def main() -> None:
    path = Path(sys.argv[1] if len(sys.argv) > 1 else "qemu.log")
    log = path.read_text(errors="replace")

    if "Exception:" in log:
        fail("unexpected kernel/user exception detected during IPC proof")

    sequences = {
        "pipe": [
            "Pipe: userspace pipe handles created",
            "Pipe: data written from userspace",
            "Pipe: data read to userspace",
            "Pipe: EOF observed after writer close",
            "Userspace: PIPE handle lifecycle OK",
        ],
        "event-timeout-and-cross-thread-wake": [
            "Event: userspace manual-reset event created",
            "Event: unsignaled wait blocked with timeout",
            "Scheduler: timed blocked task woke",
            "Thread: additional user thread created",
            "Event: blocking wait registered",
            "Event: SIGNAL woke blocked user task",
            "Userspace: secondary thread signaled event",
            "Userspace: cross-thread EVENT wake OK",
            "Event: signaled wait completed immediately",
            "Event: manual-reset event reset",
            "Userspace: EVENT wait/timeout lifecycle OK",
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
        "local-service-cross-process": [
            "Service: named local endpoint registered",
            "Service: client connected by name",
            "Service: message sent through local transport",
            "Service: message received through local transport",
            "Userspace: child local service round-trip OK",
            "Userspace: cross-process local service round-trip OK",
        ],
    }

    for name, markers in sequences.items():
        require_ordered(log, name, markers)


if __name__ == "__main__":
    main()
