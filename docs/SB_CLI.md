# SB CLI

The `sb` command is a first-class interface. The graphical SB Store and Settings UI must call the same underlying APIs rather than maintaining separate behavior.

## Core commands

```text
sb search <query>
sb info <package>
sb install <package> [variant]
sb remove <package>
sb update [package]
sb list
sb clean [safe|recommended|expert]
sb doctor
sb settings get <key>
sb settings set <key> <value>
sb service list
sb service enable <name>
sb service disable <name>
sb process list
sb repo list
sb repo add <url>
sb recovery list
sb recovery rollback <id>
```

## Non-negotiable behavior

- Every common GUI Store operation has an equivalent CLI operation.
- Output supports human-readable and machine-readable modes.
- Automation must not require parsing decorative UI text.
- Errors include a stable error code, a clear message, and a recovery hint.
- Package actions are transactional where practical and support rollback.
- `sb clean safe` may remove only data classified as rebuildable by the system ownership database.

## Scripts and automation

```text
sb --json list
sb --json info org.suirabox.minecraft
sb --yes install org.suirabox.minecraft
```

The CLI is especially important for Server and DataCenter editions, recovery environments, remote administration, and advanced users.
