# SuiraBox Error UX Specification

## Principles

1. Normal application and service failures must not look like kernel failures.
2. The error UI must explain what happened, what is still running, and what the user can do next.
3. Every serious error receives a stable Error ID that support staff can search.
4. Diagnostic details must be available without forcing users to understand kernel internals.
5. Crash reports must minimize exposure of private user data and explicitly redact sensitive fields.
6. Recovery is preferred over shutdown whenever the affected subsystem can be isolated safely.

## Severity model

- `INFO`: informational event; no action required.
- `NOTICE`: noteworthy but harmless state change.
- `WARNING`: degraded state; normal operation continues.
- `ERROR`: one component failed; OS should continue where possible.
- `CRITICAL`: major subsystem failure; recovery is attempted.
- `RECOVERY`: boot/runtime recovery path is active.
- `PANIC`: kernel cannot safely continue.

## Normal error UI

Normal errors should use an in-session dialog/notification rather than an OD screen.

Show:

- Human-readable title
- What failed
- Impact on the user
- Current recovery action
- Recommended next action
- `Error ID`
- `Details` button
- `Open support report` button when applicable

Example:

> GPU driver restarted
> The graphics driver stopped responding and was restarted. Your other applications are still running.
> Error ID: `GPU-DRV-0007`
> [Details] [Open support report]

## BSOD semantics

BSOD means the kernel was running, but continuing would be unsafe or logically invalid. The system stops instead of risking corruption.

Conceptually:

> **"I was running, but I cannot safely continue."**

BSOD is a last-resort kernel panic presentation, not a normal notification.

BSOD should display:

- `SUIRABOX KERNEL PANIC`
- Severity / panic class
- Stable `Error ID`
- Exception name and vector
- Error code when supplied by the CPU
- `RIP`, `CS`, `RFLAGS`, `CR2` when available
- Current process/thread ID when safe to obtain
- Kernel build/version
- Boot session ID
- Recent subsystem/module name
- Recovery status
- A short human explanation
- A support/debug code block intended for copying or photographing

The screen should also offer a clearly marked diagnostic path where the platform supports it, for example saving a crash record to a recovery area. It must never imply that the user should keep using the system after an unsafe kernel state.

## RSOD semantics

RSOD is the emergency/recovery renderer. It is used when normal OS graphics/output or normal boot/recovery services are unavailable, or when boot-critical data is corrupted enough that the regular UI cannot be trusted.

Conceptually:

> **"The normal display/recovery path itself is compromised, or the boot data is not trustworthy."**

RSOD is not a generic second BSOD. It has two intended classes:

1. **Output emergency**: normal framebuffer/compositor/display path failed, so only the emergency renderer can be used.
2. **Boot integrity emergency**: boot-critical files or metadata failed integrity checks, so the system enters a minimal recovery environment.

RSOD should be intentionally sparse and extremely robust. It should depend on as little code as possible.

## Diagnostic report

For `ERROR` and above, the user may open a detailed report. For `CRITICAL`/`PANIC`, the report should be generated automatically when safe.

Suggested record structure:

```text
SB Diagnostic Report
Error ID: SB-KERNEL-XXXXXXXX
Severity: PANIC
Timestamp: <UTC>
Boot Session: <id>
OS Build: <version + commit>
Architecture: x86_64
Subsystem: <name>
Component: <name>

CPU:
  Vendor: <redacted/optional>
  Features: <summary>

Memory:
  Total: <bytes>
  Available: <bytes>

Fault:
  Vector: <value>
  Error Code: <value>
  RIP: <value>
  CS: <value>
  RFLAGS: <value>
  CR2: <value>

Recent Events:
  ...

Recovery Attempt:
  <result>

Recommended Action:
  <plain-language action>
```

## Support workflow

A user should be able to send one report to support instead of guessing what information matters.

The support report must:

- Include the stable Error ID.
- Include exact OS build information.
- Include relevant hardware/driver identifiers.
- Include the failure subsystem and recent events.
- Redact passwords, tokens, private keys, browser session data, file contents, and other user secrets.
- Clearly state what was redacted.
- Be exportable as text first; a machine-readable format can be added later.

## Testing

The project should include explicit developer-only test paths for:

- Normal error notification
- ERROR details dialog
- CRITICAL recovery screen
- BSOD rendering
- RSOD rendering
- Panic data formatting
- Report redaction
- Recovery boot path

A normal user boot must never intentionally enter BSOD/RSOD because of a test hook or ordinary application error.
