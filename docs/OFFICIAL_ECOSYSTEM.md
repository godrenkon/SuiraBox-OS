# Suiram Official Ecosystem

## Purpose

Suiram is the project/community identity. SuiraBox OS is the flagship operating-system project under Suiram.

The official ecosystem should remain usable with zero paid infrastructure wherever practical.

## Canonical source of truth

The official website published through GitHub Pages is the public entry point and canonical directory for official links.

Canonical destinations:

- Website: GitHub Pages
- Source: GitHub
- Releases: GitHub Releases, linked from the website
- Documentation: GitHub repository/docs, linked from the website
- Community: official Discord, linked from the website
- Announcements: official X account, linked from the website

The project should never require users to guess which social account, download mirror, or Discord server is official.

## Naming

Mother project/community:

`Suiram Project`

Operating system:

`SuiraBox OS`

Community account target:

`@Suiram_Cmty`

The account name is treated as an official identity only after the link is published from the canonical website.

## Official website structure

```text
/
├── Download
├── Releases
├── Features
├── Hardware
├── Editions
├── Minecraft
├── Server
├── Documentation
├── Security
├── Roadmap
├── Community
├── Contributing
└── About Suiram
```

## Distribution principles

The website should remain small and fast. Large ISO files, package payloads, videos, and other binary assets should not be embedded in the website repository when a release or artifact link can be used instead.

The site should show the exact file name, version, architecture, edition, size, checksum, release notes, and verification instructions before a download.

## Community trust

Only links published on the canonical website are official.

Community-made projects may be listed separately as community projects. They must not be presented as official merely because they use the Suiram or SuiraBox name.

## Long-term structure

```text
Suiram Project
├── SuiraBox OS
├── SB Store / Repository
├── SB Runtime / JVM work
├── SB SDK
├── Documentation
├── Official Website
└── Community
    ├── Official Discord
    ├── Official X
    └── Community projects
```

## Release integrity

Every public release should have a stable release page and machine-readable metadata. Future releases should support checksums, signatures, reproducible build information where practical, and rollback/recovery documentation.
