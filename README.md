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
- CUI専用版は現在作らず、GUI版SB Desktopを完成させる
- 大型resourceは原則としてOS本体へ埋め込まず、必要時に取得・検証・cacheする

## Initial target

開発初期は **x86_64 + QEMU** をターゲットとし、GitHub Actionsでビルド・テストできる環境を構築します。

最初のマイルストーンは、ブート可能なイメージを作り、最小Cカーネルを起動して画面へメッセージを表示することです。

## Roadmap

Roadmapは大項目だけでなく、実装単位を追跡できるよう **章（Chapter）→話（Episode）** の2段階で管理します。

### Status legend

- `[x]` 完了を確認済み
- `[-]` 実装中・部分完了
- `[*]` 現在の主作業地点
- `[ ]` 未着手

> **Current focus:** **5-3 / 5-4 → 6-3**
>
> Schedulerのtimer preemption、保存済みregister frameからのresume、CR3/TSSを含むuser-thread実行経路を完成させ、Process / Threadからfirst user threadを実際に起動できる状態へ進めています。
>
> 仕様書上の記述は実装完了を意味しません。ソース、build、test、CI、QEMU、実機検証を優先します。

## 1. Bootloader

- [x] **1-1** ブート可能イメージ生成
- [x] **1-2** kernel entryへの制御移行
- [x] **1-3** Multiboot2 boot information受け渡し
- [x] **1-4** boot情報のgeometry / memory validation
- [x] **1-5** early console / framebuffer bootstrap
- [ ] **1-6** UEFI / BIOS boot path整理
- [ ] **1-7** boot failure / recovery path

## 2. Minimal C Kernel

- [x] **2-1** C kernel基本起動
- [x] **2-2** early logging / diagnostics
- [x] **2-3** CPU initialization foundation
- [x] **2-4** GDT / TSS foundation
- [x] **2-5** panic / exception diagnostics foundation
- [-] **2-6** kernel runtime foundation hardening
- [ ] **2-7** per-CPU runtime foundation

## 3. Interrupt / Timer

- [x] **3-1** IDT initialization
- [x] **3-2** exception entry
- [x] **3-3** IRQ entry / exit
- [x] **3-4** timer initialization
- [x] **3-5** canonical timer API
- [*] **3-6** timer → scheduler preemption接続
- [ ] **3-7** sleep / wake / timeout semantics
- [ ] **3-8** multiple timer source support

## 4. Physical / Virtual Memory

- [x] **4-1** physical memory map取得
- [x] **4-2** PMM reserve / allocate / free
- [x] **4-3** page table / VMM foundation
- [x] **4-4** kernel address space維持
- [x] **4-5** user address space foundation
- [-] **4-6** processごとのCR3切替検証
- [-] **4-7** user stack / kernel stack boundary
- [ ] **4-8** advanced VM / copy-on-write
- [ ] **4-9** memory pressure / reclamation

## 5. Scheduler

- [x] **5-1** runnable thread model
- [-] **5-2** context object / switch ABI
- [*] **5-3** timer preemption path
- [*] **5-4** saved register frame → thread resume
- [ ] **5-5** RUNNABLE / BLOCKED / SLEEPING transition completion
- [ ] **5-6** sleep / wake scheduler integration
- [ ] **5-7** CPU accounting
- [ ] **5-8** priority / affinity foundation
- [ ] **5-9** SMP scheduling / load balancing
- [ ] **5-10** Minecraft workload-aware policy

## 6. Process / Thread

- [x] **6-1** thread object / lifecycle foundation
- [-] **6-2** process object / address space binding
- [*] **6-3** first user thread launch path
- [ ] **6-4** spawn / fork-equivalent process creation
- [ ] **6-5** process exit / cleanup completion
- [ ] **6-6** wait / parent-child lifecycle
- [ ] **6-7** multiple thread / multiple process execution
- [ ] **6-8** cancellation / signal-equivalent control
- [ ] **6-9** process resource accounting / limits

## 7. Syscall / IPC

- [ ] **7-1** syscall entry / return ABI
- [ ] **7-2** syscall numbering / ABI versioning
- [ ] **7-3** user pointer validation
- [ ] **7-4** handle / descriptor foundation
- [ ] **7-5** pipe
- [ ] **7-6** message queue
- [ ] **7-7** event / wait object
- [ ] **7-8** shared memory + permission
- [ ] **7-9** local service transport

## 8. VFS / Storage

- [x] **8-1** VFS write/read contract foundation
- [x] **8-2** FAT32 mount / root lookup foundation
- [x] **8-3** FAT32 existing-file write
- [x] **8-4** FAT32 new root-file creation
- [x] **8-5** FAT1/FAT2 update and allocation rollback
- [-] **8-6** filesystem abstraction expansion
- [ ] **8-7** block device layer completion
- [ ] **8-8** cache / writeback / fsync semantics
- [ ] **8-9** storage recovery

## 9. Network Stack

- [ ] **9-1** network device abstraction
- [ ] **9-2** Ethernet
- [ ] **9-3** ARP / Neighbor Discovery
- [ ] **9-4** IPv4 / IPv6
- [ ] **9-5** UDP
- [ ] **9-6** TCP
- [ ] **9-7** DNS client
- [ ] **9-8** socket API
- [ ] **9-9** firewall / isolation foundation

## 10. Driver Framework

- [ ] **10-1** device model
- [ ] **10-2** bus abstraction
- [ ] **10-3** PCI / PCIe enumeration
- [ ] **10-4** interrupt routing integration
- [ ] **10-5** storage drivers
- [x] **10-6** PS/2 keyboard / mouse foundation
- [ ] **10-7** USB framework
- [ ] **10-8** audio devices
- [ ] **10-9** power-management devices

## 11. Graphics

- [-] **11-1** framebuffer abstraction
- [ ] **11-2** display device model
- [x] **11-3** input / display event transport foundation
- [ ] **11-4** graphics memory management
- [x] **11-5** compositor foundation
- [-] **11-6** window system
- [-] **11-7** font / text rendering
- [ ] **11-8** GPU acceleration path
- [ ] **11-9** multi-monitor

## 12. Runtime / JVM Integration

- [ ] **12-1** userspace runtime foundation
- [ ] **12-2** process launch integration
- [ ] **12-3** JVM distribution / selection model
- [ ] **12-4** JVM resource limits
- [ ] **12-5** JVM memory / scheduler policy hooks
- [ ] **12-6** Java application integration
- [ ] **12-7** crash / diagnostic integration

## 13. SB Hub

- [ ] **13-1** application/service discovery
- [ ] **13-2** package metadata model
- [ ] **13-3** install / remove UI
- [ ] **13-4** update UI
- [ ] **13-5** permission / trust display
- [ ] **13-6** logs / diagnostics UI
- [ ] **13-7** recovery UI

## 14. Minecraft Runtime / Instance Manager

- [ ] **14-1** instance model
- [ ] **14-2** version / loader metadata
- [ ] **14-3** isolated instance storage
- [ ] **14-4** Java runtime selection
- [ ] **14-5** resource / memory configuration
- [ ] **14-6** launch / stop lifecycle
- [ ] **14-7** performance policy integration

## 15. Minecraft Server Manager

- [ ] **15-1** server instance model
- [ ] **15-2** server runtime selection
- [ ] **15-3** start / stop / restart lifecycle
- [ ] **15-4** console / log transport
- [ ] **15-5** resource limits
- [ ] **15-6** backup / recovery integration
- [ ] **15-7** performance policy integration

## 16. Performance / Benchmark Suite

- [ ] **16-1** boot-time benchmarks
- [ ] **16-2** scheduler latency benchmarks
- [ ] **16-3** syscall overhead benchmarks
- [ ] **16-4** memory allocation benchmarks
- [ ] **16-5** storage throughput / latency benchmarks
- [ ] **16-6** graphics / compositor benchmarks
- [ ] **16-7** Minecraft workload benchmarks
- [ ] **16-8** regression baseline / CI thresholds

## Status

**Active product: SB Desktop.**

The project is intentionally built in small, testable stages. A roadmap item is not considered complete merely because source files or UI placeholders exist; implementation evidence must support the claimed state.

## License

License will be selected before the first public release.
