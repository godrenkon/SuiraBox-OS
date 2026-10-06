# PS/2 keyboard and userspace input foundation

The current PC/PIC path initializes the first i8042 port before STI, selects
keyboard scan set 2 with controller translation to set 1, installs IRQ1 and
unmasks only that line in addition to IRQ0. The second port stays disabled.
Command polls and explicit RESEND retries are bounded. An unavailable controller
keeps the keyboard IRQ masked and does not prevent boot. This initial driver
targets QEMU's PC keyboard; ACPI discovery, USB/HID, mice and real-PC validation
remain future work.

The IRQ stub preserves every GPR, clears DF and aligns the C stack, drains at
most 32 hardware bytes, then acknowledges the master PIC. No allocations, user
copies, serial rendering or scheduler work run in the keyboard IRQ. AUX bytes
are ignored; parity/timeout errors reset held state and publish a loss event.

## Event contract

`KEY_EVENT_READ` (45) is nonblocking and restricted to init/PID 1, matching the
current display owner. Supply a writable 32-byte `sb_key_event_t`, its exact size
and zero flags. Rights are checked before pointer access. A valid empty read
returns WOULD_BLOCK; an unavailable keyboard returns NOT_FOUND. Validate the
complete output range even when empty; copy the event before removing it. A
failed output copy leaves the queued event intact. Current UP interrupt gates
serialize IRQ producers and syscall consumers; this is not an SMP queue design.

The event contains size/version, KEY or OVERFLOW type, a normalized physical
US-labelled position, DOWN/REPEAT flags, modifier snapshot, monotonic 64-bit
sequence, cumulative discarded-entry count and zero reserved field. Letter
positions use uppercase ASCII codes; punctuation/digits use their unshifted US
labels. Special positions have named values starting at 256. No controller
scancode, pointer or kernel-generated text is exposed. The modifier snapshot is
after each event: either Shift/Ctrl/Alt/Meta side contributes to its aggregate
bit; Caps toggles on the first make only. Repeated make carries REPEAT, release
does not. Stray releases and unknown positions are ignored. E0 positions,
PrintScreen fake shifts and the six-byte Pause sequence are handled explicitly.

The fixed queue holds 64 entries. On overflow it discards unread entries and the
incoming key, publishes one OVERFLOW snapshot, and continues tracking live
decoder state. `dropped` counts discarded queue entries including incoming keys,
saturates at UINT32_MAX, and is repeated in subsequent KEY events. Sequence gaps
and the OVERFLOW event tell a client to reset its held-key assumptions; the
client must not infer missing key releases. Hardware corruption also clears the
decoder's held state. A dedicated compositor/input capability and blocking waits
remain future work; other processes cannot drain the boot owner's queue.

## Visible demo and tests

After the existing IPC/child lifecycle completes, normal init shows a 32-character
ASCII input line. The userspace editor applies a temporary US layout, Shift/Caps,
Backspace and Enter. Enter shows the last submitted line; arrows and shortcuts
do not insert text. The loop sleeps one PIT tick while empty. This is a small
keyboard demo, not a shell, desktop, IME, language dialog or Minecraft runtime.

Host tests cover controller ACK/RESEND/absence/timeouts and command ordering,
scan decoding/modifiers/repeats/extended sequences, ring wrap/overflow/error
resynchronization, argument/rights/fault preservation, bounded editing and US
layout. QEMU CI sends 22 down/up events through QMP's virtual keyboard, not an
OS injection syscall. It verifies Shift+A, B, Backspace, Caps+C, Caps off, Right,
right Ctrl/Alt and Enter, with exact ordered event metadata in ring 3. Invalid
and read-only outputs are retried while the first event is queued, then the
same sequence must survive. Before and after PNG/PPM screens are compared at
every pixel; the final submitted value is AC and the typed line is empty. The
file-backed disk SHA-256 must remain unchanged and all existing boot/IPC/storage
proofs run alongside it. INPUT_PROOF is opt-in and its C proof symbol disappears
when the mode is disabled; normal editing remains enabled.

Protocol verification references: [QEMU PC keyboard controller](https://github.com/qemu/qemu/blob/master/hw/input/pckbd.c),
[PS/2 emulation](https://github.com/qemu/qemu/blob/master/hw/input/ps2.c),
and [QMP input schema](https://github.com/qemu/qemu/blob/master/qapi/ui.json).
The SuiraBox driver and tests are independently implemented.
