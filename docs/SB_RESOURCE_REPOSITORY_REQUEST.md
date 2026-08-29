# External repository creation request

Repository to create:

`godrenkon/SuiraBox-OS-Resources`

Purpose: hold all non-boot-critical SB Desktop resources so the OS ISO remains minimal.

The OS repository already points its resource manifest URL at:

`https://raw.githubusercontent.com/godrenkon/SuiraBox-OS-Resources/main/manifest/manifest-v1.json`

Required first-wave resource families:

```text
locale
keyboard
font
theme
wallpaper
icon
sound
app
``` 

Each payload must be independently versioned and SHA-256 identified. The resource repository must never become a mirror of the OS source tree or contain the kernel/bootloader as a normal resource.

This file records the required external repository because the connected GitHub tool currently does not expose a repository-creation operation.
