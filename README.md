[https://github.com/godrenkon/SuiraBox-OS/issues/7#issue-5307409508](https://github.com/godrenkon/SuiraBox-OS/issues/7)
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

Roadmapは、旧来の大項目だけでは現在地が分かりにくいため、**章（Chapter）→話（Episode）** の2段階で管理します。

### Status legend

- `[x]` 完了を確認済み
- `[-]` 実装中・部分完了
- `[ ]` 未着手
- `[*]` 現在の主作業地点

> **Current focus:** **5-3 / 5-4 → 6-3**。Schedulerの実コンテキスト切替・timer preemptionを完成させつつ、Process / Thread層のuser thread起動経路へ接続しています。
>
> 仕様上の完成と実装上の完成は同一ではありません。実際の状態はソースコード、build、test、CI、QEMU、実機検証を優先します。

---

## 1. Bootloader

- [x] **1-1** ブート可能イメージ生成
- [x] **1-2** kernel entryへの制御移行
- [x] **1-3** boot informationの受け渡し
- [x] **1-4** kernelが扱うboot情報の正規化
- [x] **1-5** 早期console / framebuffer基盤
- [ ] **1-6** UEFI / BIOS経路の整理と拡張
- [ ] **1-7** boot failure / recovery経路

## 2. Minimal C Kernel

- [x] **2-1** C kernelの基本起動
- [x] **2-2** kernel logging / early diagnostics
- [x] **2-3** CPU初期化の基礎
- [x] **2-4** GDT / TSS基盤
- [x] **2-5** kernel panic / fault diagnostics基盤
- [-] **2-6** kernel runtime基盤の整理
- [ ] **2-7** per-CPU runtime基盤

## 3. Interrupt / Timer

- [x] **3-1** IDT初期化
- [x] **3-2** CPU exception entry
- [x] **3-3** IRQ entry / exit
- [x] **3-4** timer初期化
- [x] **3-5** canonical timer API
- [-] **3-6** timerからschedulerへのpreemption接続
- [ ] **3-7** sleep / wake / timeout semanticsの完全化
- [ ] **3-8** 複数timer sourceへの適応

## 4. Physical / Virtual Memory

- [x] **4-1** physical memory map取得
- [x] **4-2** PMMのfree / reserve / allocate基盤
- [x] **4-3** page table / VMM基盤
- [x] **4-4** kernel address space維持
- [x] **4-5** user address space基盤
- [-] **4-6** processごとのCR3切替検証
- [-] **4-7** user stack / kernel stack境界整理
- [ ] **4-8** copy-on-write / advanced VM features
- [ ] **4-9** memory pressure / reclamation

## 5. Scheduler

- [x] **5-1** runnable threadの基本モデル
- [-] **5-2** context object / switch ABI
- [*] **5-3** timer preemption経路
- [*] **5-4** 保存済みregister frameからのthread resume
- [ ] **5-5** runnable / blocked / sleeping遷移の完全化
- [ ] **5-6** sleep / wakeとschedulerの統合
- [ ] **5-7** CPU accounting
- [ ] **5-8** priority / affinity基盤
- [ ] **5-9** SMP scheduling / load balancing
- [ ] **5-10** Minecraft workload向けpolicy

## 6. Process / Thread

- [x] **6-1** thread object / lifecycleの基本
- [-] **6-2** process object / address space紐付け
- [*] **6-3** first user thread起動経路
- [ ] **6-4** user threadのfork / spawn相当
- [ ] **6-5** process exit / cleanup
- [ ] **6-6** wait / parent-child lifecycle
- [ ] **6-7** 複数thread / 複数process実行
- [ ] **6-8** signal / cancellation相当
- [ ] **6-9** resource accounting / limits

## 7. Syscall / IPC

- [ ] **7-1** syscall entry/exit ABI
- [ ] **7-2** syscall番号・ABI versioning
- [ ] **7-3** user pointer validation
- [ ] **7-4** handle / descriptor基盤
- [ ] **7-5** pipe
- [ ] **7-6** message queue
- [ ] **7-7** event / wait object
- [ ] **7-8** shared memory + permission
- [ ] **7-9** local service transport

## 8. VFS / Storage

- [ ] **8-1** VFS object model
- [ ] **8-2** path / mount semantics
- [ ] **8-3** file / directory API
- [ ] **8-4** block device layer
- [ ] **8-5** filesystem abstraction
- [ ] **8-6** 初期filesystem実装
- [ ] **8-7** cache / writeback
- [ ] **8-8** fsync / atomic update semantics
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
- [ ] **10-6** input drivers
- [ ] **10-7** USB framework
- [ ] **10-8** audio devices
- [ ] **10-9** power-management devices

## 11. Graphics

- [ ] **11-1** framebuffer abstraction
- [ ] **11-2** display device model
- [ ] **11-3** input/display event transport
- [ ] **11-4** graphics memory management
- [ ] **11-5** compositor foundation
- [ ] **11-6** window system
- [ ] **11-7** font / text rendering
- [ ] **11-8** GPU acceleration path
- [ ] **11-9** multi-monitor

## 12. Runtime / JVM Integration

- [ ] **12-1** userspace runtime foundation
- [ ] **12-2** process launch from runtime
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

- [ ] **14-1** Minecraft installation model
- [ ] **14-2** instance management
- [ ] **14-3** version management
- [ ] **14-4** mod / loader integration
- [ ] **14-5** Java runtime selection
- [ ] **14-6** per-instance resource policy
- [ ] **14-7** launch / crash diagnostics
- [ ] **14-8** backup / restore workflow

## 15. Minecraft Server Manager

- [ ] **15-1** server instance model
- [ ] **15-2** server runtime management
- [ ] **15-3** resource limits
- [ ] **15-4** port / network configuration
- [ ] **15-5** console / log viewer
- [ ] **15-6** backup / restore
- [ ] **15-7** update / version management
- [ ] **15-8** server monitoring

## 16. Performance / Benchmark Suite

- [ ] **16-1** benchmark framework
- [ ] **16-2** kernel microbenchmarks
- [ ] **16-3** scheduler benchmarks
- [ ] **16-4** memory benchmarks
- [ ] **16-5** storage benchmarks
- [ ] **16-6** network benchmarks
- [ ] **16-7** graphics benchmarks
- [ ] **16-8** JVM / Minecraft workloads
- [ ] **16-9** regression thresholds
- [ ] **16-10** reproducible performance reports

## 17. SB Desktop / System UX

- [ ] **17-1** userspace init / service manager
- [ ] **17-2** display/input services
- [ ] **17-3** desktop shell
- [ ] **17-4** window / workspace management
- [ ] **17-5** Settings
- [ ] **17-6** File Manager
- [ ] **17-7** Terminal
- [ ] **17-8** notifications / system tray
- [ ] **17-9** account / privacy / security UI
- [ ] **17-10** first-run setup
- [ ] **17-11** recovery UX

## 18. Security / Reliability / Release

- [ ] **18-1** privilege separation
- [ ] **18-2** memory / process isolation review
- [ ] **18-3** syscall security audit
- [ ] **18-4** driver isolation / failure containment
- [ ] **18-5** filesystem consistency testing
- [ ] **18-6** fuzzing / negative testing
- [ ] **18-7** QEMU integration tests
- [ ] **18-8** real-hardware validation
- [ ] **18-9** release image reproducibility
- [ ] **18-10** release criteria / support matrix

---

## Current Development Position

### Primary work

**5-3 → 5-4: timer preemption + saved-frame resume**

現在は、timer IRQから現在threadの実行状態を扱い、次に実行するthreadの保存済みframeへ切り替える、Schedulerの核心部分を実装・検証しています。

### Parallel integration

**6-2 → 6-3: Process / Thread + first user thread**

同時に、processごとのaddress space、kernel stack、user stack、復帰用frame、TSS.rsp0、CR3切替をつないで、最初のuser threadを実際に起動できる経路を完成させています。

### Next boundary

5章と6章が安定した後は、**7-1 syscall entry/exit ABI**へ進み、userspaceとkernelの正式な境界を固定します。

---

## Status

Early development / kernel runtime construction.

The project is intentionally built in small, testable stages instead of attempting the complete OS at once.

## License

License will be selected before the first public release.
