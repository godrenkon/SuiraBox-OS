# Core Rule

Do not externalize a component merely because it can be downloaded.

Keep it in Core when it is required for:

- boot;
- basic GUI operation;
- local storage and recovery;
- network/resource-client startup;
- security/integrity verification;
- recovery terminal or minimal settings;
- functionality that users cannot meaningfully opt out of;
- a sufficiently small payload where externalization would cost more complexity than it saves.

External Resources are for user-selectable, replaceable, large, or rarely-needed content.

A working offline Core is mandatory.
