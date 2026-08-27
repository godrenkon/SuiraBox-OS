# SuiraBox OS

**Minecraftに特化しながら、普段使いもできるオープンソースOSを目指すプロジェクト。**

> High performance. Simple to use. Minecraft-first.

## Vision

SuiraBox OS (SB) は、Minecraft Java Editionを中心に、ゲーム、Minecraft Server、ブラウジング、開発などを快適に扱える汎用デスクトップOSを目指します。

Minecraft専用OSに閉じるのではなく、一般的なPC用途を維持したうえで、Minecraft向けのScheduler、Memory、Storage、Network、Graphics、JVM Runtimeなどを最適化できるアーキテクチャを設計します。

## Architecture

```text
Applications
    |
    +-- SB Hub / Desktop
    +-- Minecraft / Minecraft Server
    +-- General Applications
    |
    v
SB Runtime / JVM
    |
    v
SB Kernel
    +-- Scheduler
    +-- Memory Manager
    +-- Process / Thread
    +-- Syscall / IPC
    +-- VFS / Storage
    +-- Network
    +-- Drivers / Graphics
    |
    v
Hardware
```

## Development principles

- Kernelは汎用性を保つ
- Minecraft向け最適化はPolicy / Runtime層として実装する
- 性能改善はベンチマークで検証する
- セキュリティと安定性を性能と同等に重視する
- 可能な限り標準仕様・既存の実績ある技術を利用する
- オープンソースで設計・開発・検証を公開する

## Initial target

開発初期は **x86_64 + QEMU** をターゲットとし、GitHub Actionsでビルド・テストできる環境を構築します。

最初のマイルストーンは、ブート可能なイメージを作り、最小Cカーネルを起動して画面へメッセージを表示することです。

## Roadmap

- [ ] Bootloader
- [ ] Minimal C Kernel
- [ ] Interrupt / Timer
- [ ] Physical / Virtual Memory
- [ ] Scheduler
- [ ] Process / Thread
- [ ] Syscall / IPC
- [ ] VFS / Storage
- [ ] Network Stack
- [ ] Driver Framework
- [ ] Graphics
- [ ] Runtime / JVM integration
- [ ] SB Hub
- [ ] Minecraft Runtime / Instance Manager
- [ ] Minecraft Server Manager
- [ ] Performance / Benchmark suite

## Status

Early development / architecture phase.

The project is intentionally built in small, testable stages instead of attempting the complete OS at once.

## License

License will be selected before the first public release.
