# SB Settings Architecture

SB Settings is a graphical configuration center intended to expose fine-grained control without making the interface difficult to understand.

## Navigation

Settings are organized around user goals:

- Performance
- Gaming
- Minecraft
- Graphics
- Network
- Storage
- Power
- Privacy & Security
- Updates
- Appearance
- Accessibility
- Developer
- Server

A global search field must find settings by user-facing names, aliases, and technical identifiers.

## Three levels

### Basic

Common controls with plain-language explanations.

### Advanced

More detailed controls with resource-impact descriptions.

### Expert

Low-level controls for administrators and developers. Dangerous changes require a clear warning and recovery path.

## User choice

Optional services should expose controls for startup behavior, background activity, permissions, and resource limits where technically possible.

Examples:

- Start with system
- Run only when needed
- Allow background operation
- CPU priority
- Memory limit
- Network access
- GPU access
- Storage locations

## Performance visibility

The UI should show which optional components consume CPU, RAM, storage, GPU, or network resources. A user should be able to identify unnecessary background activity without reading process tables.

## Transactional changes

Settings that affect boot, drivers, storage, network, or security should be applied through a transaction layer where practical. The previous configuration should be retained until the new state is known to work.

## Profiles

Settings can be saved as user profiles. Examples:

- Balanced
- Maximum Gaming
- Minecraft Performance
- Battery Saver
- Server
- Custom

Profiles should be exportable as declarative configuration so advanced users can share them and GUI users can import them without editing files manually.
