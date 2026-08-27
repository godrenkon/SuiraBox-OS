# SuiraBox OS — Complete Master Specification

> **Document role:** SB Desktop の最上位仕様書・全体設計書・制作計画・復旧用バックアップ。
>
> このファイルは「SB OSとは何を作るのか」を最初から最後まで復元するための唯一の最上位資料として扱う。通常の取扱説明書より深く、ユーザー体験、OS内部、データ、状態、境界、失敗時の挙動、実装順序、テスト、リリース、将来拡張までを記録する。
>
> **現在の製品対象:** SB Desktop
>
> **現在の開発対象外:** 独立したCUI専用版OS。SB Desktop内部のTerminal/CLIは必須機能として実装する。
>
> **重要:** この文書は「完成品の証明」ではない。実装状態はソースコード、ビルド、テスト、CI、QEMU、実機検証で判定する。

---

# 1. SBとは何か

## 1.1 製品名

- Project: **SuiraBox OS**
- Short name: **SB**
- Desktop edition: **SB Desktop**
- Community/project identity: **Suiram**
- Repository: `godrenkon/SuiraBox-OS`

## 1.2 SBの目的

SBは、可能な限り余計なものを削りながら、普通のPCユーザーにも高度なユーザーにも使いやすい、オープンソースの汎用デスクトップOSを作るプロジェクトである。

SBは特定の操作方法や不要な常駐機能をユーザーへ強制しない。基本OSを小さくし、必要な機能を後から選択して追加できる構造を採用する。

目標は「機能が多いOS」そのものではない。目標は、**必要なものだけを軽く使え、必要なら深く制御でき、壊れたときに原因が分かり、可能な範囲で復旧できるOS**である。

## 1.3 設計思想

SBの中心思想は次の通り。

- 最小構成を基本とする。
- オプション機能は必要な時に追加する。
- 常時起動する不要なサービスを増やさない。
- 設定を細かくできるが、画面自体は分かりやすくする。
- GUIを通常利用の中心にする。
- Terminal/CLIを完全に捨てない。
- GUIとCLIは可能な限り同じ基盤/APIを使用する。
- 普通のエラーと致命的エラーを明確に分ける。
- 技術情報を必要な人には詳細に見せる。
- ユーザーデータを勝手に削除しない。
- ダウンロードと更新を可能な限り軽く・速くする。
- ハードウェア非依存の基盤を先に作り、その後にGPU等へ拡張する。
- 性能は測定値で判断する。
- セキュリティ、安定性、データ保全を性能と同等以上に重視する。
- 「動いているように見えるだけ」のモックを完成扱いしない。

## 1.4 「RPGツクール」のようなOSという考え方

SBでは、ベースOSを小さな土台として、その上にユーザーが必要な機能を追加できる感覚を重視する。

```text
最小OS
  ↓
必要な機能を選ぶ
  ↓
必要なコンポーネントだけ取得
  ↓
自動設定
  ↓
使用
```

ただし、すべてを無意味に細かいパッケージへ分割しない。分割には、サイズ、RAM、更新、安全性、保守性、ユーザー選択性の実益が必要である。

---

# 2. SB Desktopの完成形

## 2.1 完成時の基本体験

ユーザーは、対応PCへSBをインストールし、電源投入後にGUIデスクトップまで到達し、Terminalを使わず通常の初期設定を完了できる。

基本経路は次の通り。

```text
電源投入
 ↓
BIOS/UEFI
 ↓
Bootloader
 ↓
Kernel early boot
 ↓
CPU / Memory / Interrupt infrastructure
 ↓
Kernel services
 ↓
Userspace init
 ↓
Storage / Configuration
 ↓
Display / Input
 ↓
Compositor
 ↓
Desktop Shell
 ↓
First-run check
 ↓
Language selector
 ↓
Desktop ready
```

## 2.2 通常ユーザーができること

完成版SB Desktopでは少なくとも次をGUIから行える。

- 起動、再起動、シャットダウン
- アプリケーション起動・終了
- ウィンドウ操作
- キーボード・マウス操作
- ファイル作成・移動・コピー・削除・名前変更
- ネットワーク設定
- 言語設定
- キーボードレイアウト設定
- ディスプレイ設定
- サウンド設定
- システム詳細設定
- ユーザー・権限管理
- ソフトウェア検索・インストール・更新・削除
- エラー通知の確認
- サポートレポート生成
- 回復操作
- Terminal/CLI利用

## 2.3 高度なユーザーができること

- TerminalからOS機能を操作
- パッケージ管理
- システムログ閲覧
- 診断情報取得
- プロセス管理
- ネットワーク診断
- ストレージ診断
- 開発者設定
- Recovery操作
- 詳細な性能情報取得

---

# 3. OS全体の層構造

```text
Firmware
  ├─ BIOS
  └─ UEFI
        ↓
Bootloader
        ↓
Early kernel
        ↓
Architecture layer
        ├─ CPU
        ├─ GDT/TSS/IDT
        ├─ Paging
        ├─ Interrupts
        ├─ Timer
        ├─ ACPI
        └─ SMP
        ↓
Kernel core
        ├─ PMM
        ├─ VMM
        ├─ Heap
        ├─ Scheduler
        ├─ Process/Thread
        ├─ IPC
        ├─ Syscall
        ├─ Security
        ├─ Logging
        └─ Panic/Recovery
        ↓
Device subsystem
        ├─ PCI
        ├─ Storage
        ├─ USB
        ├─ Input
        ├─ Display/GPU
        ├─ Network
        ├─ Audio
        └─ Power
        ↓
Filesystem / VFS
        ↓
Userspace
        ├─ Init / Service Manager
        ├─ Configuration service
        ├─ Display service
        ├─ Input service
        ├─ Network manager
        ├─ Package manager
        ├─ Update service
        ├─ Logging/diagnostic service
        ├─ Account service
        └─ Notification service
        ↓
GUI platform
        ├─ Font / text
        ├─ Toolkit
        ├─ Compositor
        ├─ Window system
        └─ Desktop Shell
        ↓
Applications
        ├─ Settings
        ├─ File Manager
        ├─ Terminal
        ├─ SB Store
        └─ Other applications
```

下位層は上位層に依存しない。特にKernelはGUIを前提にしない。

---

# 4. ハードウェア対象と互換性方針

## 4.1 初期ターゲット

最初の安定ターゲットは、**generic x86_64 PC + QEMU** とする。

特定ベンダー製PCだけを前提にしない。

## 4.2 将来的な対応範囲

基盤完成後、同じSBアーキテクチャから段階的に対応範囲を広げる。

- 一般デスクトップPC
- ノートPC
- ゲーミングPC
- ワークステーション
- 高性能GPU搭載PC
- Professional GPU搭載PC
- サーバー
- データセンター向けハードウェア

NVIDIA RTXなどを含む高性能GPUについても、将来的に実ドライバ実装と検証を行う。ただし、ドライバが未実装なのに「対応済み」と宣言しない。

## 4.3 リリース分岐方針

最初から大量のISOを作らない。

共通基盤を完成させてから、実際の互換性データに基づき、必要ならRelease artifactを用途別に分ける。

例:

```text
SB Desktop Generic
SB Desktop Gaming
SB Desktop Workstation
SB Desktop Server-oriented
```

これは第一段階の完成後に行う。

---

# 5. ブートとファームウェア

## 5.1 BIOS/UEFI

SBはファームウェアから直接ハードウェアを操作するのではなく、Bootloaderを境界として利用する。

必要な情報はOS内部形式へ正規化する。

## 5.2 Bootloaderの責務

- CPUをKernelが想定する状態へ移行
- Kernel entryを呼ぶ
- Kernel stackを準備
- Boot protocol情報を引き渡す
- 必要なboot modulesを引き渡す
- Kernel imageの範囲を明確化
- Boot関連メモリ範囲を記録

BootloaderでGUIや通常アプリケーションを起動しない。

## 5.3 Multiboot2情報のライフサイクル

Multiboot2情報そのものをPMMへ即時解放可能な領域として扱わない。

起動時は、

```text
Boot information received
 ↓
Boot information range registered as protected
 ↓
Required fields parsed/copy to owned structures
 ↓
Dependent subsystems released
 ↓
Only after final user is gone, original storage may become reusable
```

とする。

UEFI Memory Map等を使用する場合も同様に、情報を参照している間は保護する。

## 5.4 Boot failure

- Kernel entry前の失敗 → Boot diagnostic path
- Kernel初期化中の致命的失敗 → Panic/RSOD decision
- 通常ユーザー向けエラーUIをEarly Bootへ強制しない

---

# 6. CPUアーキテクチャ

## 6.1 実装言語

Kernelの中心は **C** とする。

低レイヤでは **x86_64 Assembly** を使用する。

必要に応じてLinker Script、Build/CI用スクリプトを使用する。

## 6.2 Assemblyの使用範囲

- Boot entry
- CPU mode transition
- Context switch primitives
- Interrupt entry/exit
- Syscall entry
- 特定のCPU命令を直接呼ぶ箇所

Cで安全に記述できる部分を不要にAssembly化しない。

## 6.3 GDT/TSS/IDT

段階的に実装する。

- GDT: kernel/user code/data segments
- TSS: stack switching / privilege transition support
- IDT: exception / interrupt routing

状態をコード上の隠れた定数にせず、仕様化された定義として管理する。

---

# 7. 物理メモリ管理 — PMM

## 7.1 目的

PMMは物理ページの所有権を管理する。

## 7.2 基本単位

ページサイズはArchitecture定数として一箇所に定義する。x86_64初期実装では通常4 KiBページを基準とする。

## 7.3 所有権

PMMがfreeとして扱えるのは、OSが安全に利用できる物理メモリだけ。

少なくとも次を予約する。

- Kernel image
- Bootloader remnants still in use
- Boot stack
- Page tables still in use
- Multiboot2 structures still in use
- UEFI/ACPI structures still in use
- PMM bitmap itself
- Other explicitly allocated boot metadata
- Hardware reserved regions

## 7.4 Bootstrap PMM

最初は安全な固定範囲を使うBootstrap PMMでよい。

ただし最終版では固定64 MiBを「実メモリそのもの」と見なさず、Boot情報から得た本当のメモリマップを共通内部表現へ変換する。

## 7.5 PMM APIの概念

```text
pmm_init()
pmm_add_usable_range(start, end)
pmm_reserve_range(start, end)
pmm_alloc_page()
pmm_free_page(page)
pmm_total_pages()
pmm_free_pages()
```

公開APIでは戻り値・失敗条件・所有権を明確にする。

## 7.6 PMM不変条件

- 同じページを二重にfreeしない
- 予約領域を返さない
- アラインメント不正を拒否
- 範囲外を拒否
- 枯渇時に明確な失敗を返す
- 再初期化で既存の所有権を破壊しない
- allocator自身のメタデータを自分でfreeしない

---

# 8. 仮想メモリ — VMM

## 8.1 目的

Virtual AddressとPhysical Pageの対応を管理する。

## 8.2 必須機能

- Map
- Unmap
- Translate
- Protection flags
- User/kernel address separation
- Page fault integration
- Page-table allocation via PMM

## 8.3 不変条件

- User processからKernel mappingへ勝手に書き込めない
- Mapping alignmentを検証する
- Unmap後に参照できない
- Page-table pageの所有権を追跡する
- 不要なmappingを残さない

---

# 9. Kernel Heap

## 9.1 目的

小さなKernel objectや可変サイズ構造体用の動的メモリを提供する。

## 9.2 方針

- PMMと役割を分離
- Alignmentを明示
- Overflowを検証
- Allocation failureを明示
- free後の二重解放を検出可能にする
- allocator内から危険なLogging依存を作らない

## 9.3 性能

Idle時のHeap処理は発生しない。必要な時だけ動作させる。

---

# 10. CPU例外・割り込み・タイマー

## 10.1 Exception

CPU Exceptionは原因とコンテキストを保存する。

代表例:

- Divide Error
- Invalid Opcode
- General Protection Fault
- Page Fault
- Double Fault
- Machine Check

## 10.2 IRQ

デバイス割り込みは共通IRQ dispatcherへ流す。

## 10.3 PIC/APIC

初期ブートではLegacy PIC/PITを使用する経路を許容する。

将来的なSMP対応を前提に、Local APIC / IOAPICを抽象化する。

## 10.4 Timer migration

タイマーを単一の隠れた実装にしない。

```text
Early single-core
    ↓
PIT fallback
    ↓
APIC/appropriate timer when platform ready
    ↓
Scheduler clock source
```

切り替え前後でscheduler tickの時間基準が破綻しないよう、内部のmonotonic time sourceを抽象化する。

HPET等の存在を理由に必ず使う必要はない。実際の精度、負荷、互換性を測定して決定する。

---

# 11. ACPI

ACPIは電源管理だけの機能ではない。Platform topologyやCPU/device情報を得るためにも使用する。

## 11.1 対象

- Root tables
- APIC/MADT
- CPU topology
- Power management tables
- Thermal情報
- Shutdown/reboot関係

## 11.2 ライフサイクル

ACPI table memoryは参照中にPMMへ戻さない。必要な情報をowned structureへ変換してからrelease可能にする。

---

# 12. SMP / マルチコア

初期版はsingle-coreで完成させてよいが、アーキテクチャはSMPへ拡張可能にする。

後段で実装するもの:

- AP startup
- Per-CPU data
- Per-CPU scheduler state
- Interrupt affinity
- Cross-CPU IPI
- Locking
- CPU hotplug設計

Shared stateとPer-CPU stateを区別する。

---

# 13. 同期プリミティブ

Kernel内では少なくとも概念として以下を整理する。

- Spinlock
- Mutex
- Read/write lock
- Semaphore
- Wait queue
- Atomic operations
- IRQ-safe lock variants

ロック順序を文書化し、循環待ちを作らない。

IRQ contextからsleep可能なmutexを取得しない。

---

# 14. Logging / Debugging Architecture

ログはOSの基盤機能だが、低レイヤのログ出力が低レイヤ自身を壊してはならない。

## 14.1 Logging levels

- Emergency
- Alert
- Critical
- Error
- Warning
- Notice
- Info
- Debug
- Trace

## 14.2 Early boot logger

Early bootではmalloc、filesystem、GUI、networkへ依存しない。

## 14.3 Lock-free / low-risk path

PMM、scheduler、spinlock、interrupt handler等からの緊急ログは、可能な場合、固定長・事前確保リングバッファまたは直接non-blocking出力を使う。

Logging関数内で、呼び出し元と同じロックやallocatorを取り直さない。

## 14.4 Serial

QEMUの初期検証ではSerial出力を主診断経路とする。

ただしSerial writeのbusy waitがOS全体を無期限に止めないよう、診断モードと通常モードを分ける。

---

# 15. Panic / BSOD / RSOD / 通常エラー

## 15.1 階層

```text
通常エラー
 ↓
Application recovery
 ↓
Service recovery
 ↓
Subsystem recovery
 ↓
Recovery mode
 ↓
BSOD / kernel stop
 ↓
RSOD only when normal display/recovery/integrity path itself is unusable
```

## 15.2 通常エラー

普通のユーザー向け。

最低限:

- 何が起きたか
- 影響範囲
- OSはまだ使えるか
- 自動回復したか
- 次に何をすればいいか
- Error ID

「怖い画面」を出さない。

## 15.3 BSOD

意味:

> 動作していたが、Kernelが安全に継続できないため停止した。

含める情報:

- Error ID
- Failure class
- Exception/vector
- CPU error code
- Instruction pointer
- Register context
- Current process/thread
- Subsystem
- Kernel version/build
- Boot/session ID
- Crash dump state
- Recovery state
- Support report ID

表示は一般ユーザーにも意味が分かる説明と、技術情報の展開領域を分ける。

## 15.4 RSOD

意味:

> 通常の画面出力や回復機構そのもの、または起動・システム整合性が信用できない。

RSODは極めて稀であるべきで、普通のクラッシュには絶対に使用しない。

---

# 16. Scheduler

## 16.1 目的

CPU時間をthreadへ安全かつ公平に割り当てる。

## 16.2 基本

初期版:

- Kernel task
- Runnable state
- Current task
- Round-robin
- Priority field
- Timer tick

将来:

- Preemptive scheduling
- Per-CPU run queue
- Sleep/wakeup
- Priority classes
- CPU affinity
- QoS/policy

## 16.3 Minecraft等の性能最適化

Minecraft向けの最適化をKernel全体へハードコードしない。

将来的にはPolicy/runtime層へ置き、汎用OSとしての挙動を壊さない。

---

# 17. Process / Thread

Processはresources/address spaceの所有単位、Threadは実行単位として扱う。

必要な概念:

- PID
- TID
- Parent relation
- Address space
- File descriptor table
- Credentials
- Environment
- Thread state
- Exit code
- Wait/Join

Process終了時に、memory、fd、IPC object、scheduler state等を確実に解放する。

---

# 18. IPC

将来的に以下を体系化する。

- Pipe
- Message queue
- Shared memory
- Event/signal
- Socket-based IPC
- Service RPC

IPC objectにはowner/lifetimeを持たせる。

GUI、Network Manager、Package Manager、Settingsなどは、可能な限りService APIを通して通信する。

---

# 19. Syscall ABI

UserspaceとKernelの境界を明確にする。

## 19.1 原則

- Pointer validation
- Length validation
- Handle validation
- Permission check
- ABI versioning
- Stable error return
- No kernel pointer leakage

## 19.2 系統

- Process/thread
- Memory map
- File I/O
- Directory
- Time
- IPC
- Network
- Device interface
- Configuration/service access

## 19.3 CLIとGUI

CLIもGUIも、特権操作を勝手に直接実装せず、同じ正式APIを使用する。

---

# 20. Userspace Init / Service Manager

## 20.1 Init

最小限のuserspace initを起動する。

## 20.2 Service Manager

サービスには、

- name
- dependency
- startup condition
- state
- restart policy
- timeout
- resource policy

を持たせる。

不要なサービスをBoot時に常駐させない。

サービスが死んだ場合は、可能なら他の部分を巻き込まず再起動する。

---

# 21. Storage Architecture

```text
Physical device
 ↓
Block layer
 ↓
Partition layer
 ↓
Filesystem driver
 ↓
VFS
 ↓
Path/file API
 ↓
Applications/services
```

## 21.1 Block layer

HDD/SSD/NVMe/virtio等を共通化する。

## 21.2 Partition

パーティション方式を抽象化し、デバイス固有ロジックを上位へ漏らさない。

## 21.3 Filesystem

初期版は最も安全に実装可能な単純なFilesystemから開始してよい。

最終的には、

- File
- Directory
- Metadata
- Permissions
- Timestamps
- Atomic write
- Durable write
- Recovery

を提供する。

## 21.4 VFS

Path resolution、mount point、file descriptorなどを共通化する。

---

# 22. Persistent Configuration

設定はファイルやサービスごとにバラバラな形式を勝手に作らない。

設定には、

- Stable ID
- Data type
- Default
- Validation
- Persistence
- Owner
- Migration version

を持つ。

書き込みは可能な限りtransactional/atomicにする。

設定破損時には、

```text
invalid config
 ↓
validate failure
 ↓
fallback/default
 ↓
repair or setup UI
```

としてBoot loopを防ぐ。

---

# 23. Installer / Live / Recovery

## 23.1 Installer

将来的な正式Installerは、

- Boot mode detection
- Disk selection
- Partitioning
- Filesystem creation
- Base OS installation
- Bootloader installation
- First-run configuration preparation

を行う。

破壊的な操作の前に、対象ディスク・パーティション・データ消去範囲を明示する。

## 23.2 Live environment

インストール前に最低限のHardware/Network/Storageを確認できるLive環境を将来的に用意する。

## 23.3 Recovery

通常Bootが失敗した場合、Recovery pathへ入れる。

Recoveryは独立した最小システムとして設計し、通常環境の壊れた部分に依存しすぎない。

---

# 24. Display Architecture

```text
GPU/framebuffer driver
 ↓
Display backend
 ↓
Compositor
 ↓
Window system
 ↓
GUI toolkit
 ↓
Applications
```

## 24.1 Fallback

Accelerated GPU driverがなくても、可能ならgeneric framebuffer等で最低限の画面を出せるようにする。

## 24.2 Multi-monitor

将来的に、

- Display enumeration
- Resolution
- Refresh rate
- Position
- Scaling
- Primary display

を扱う。

---

# 25. Input Architecture

対象:

- Keyboard
- Mouse
- Touchpad
- Touchscreen later
- Hotkeys
- Focus

InputはHardware raw eventからOS internal eventへ変換する。

アプリケーションへ直接hardware scan codeを渡さず、共通Input APIを使用する。

---

# 26. Text / Font / Unicode / IME

多言語OSなので、単にASCIIが表示できるだけでは不十分。

## 26.1 Unicode

内部文字列処理はUnicodeを前提とする。

## 26.2 Font fallback

必要な文字が主フォントに存在しない場合、fallback fontを利用する。

## 26.3 Input Method

将来的に、

- 日本語IME
- 中文入力
- その他の入力方式

へ拡張可能なInput Method Frameworkを作る。

---

# 27. GUI Toolkit

最低限必要なwidget:

- Window
- Label
- Button
- Text field
- Checkbox
- Radio button
- Select/drop-down
- List
- Scroll view
- Menu
- Dialog
- Notification
- Progress bar
- Tabs
- Table/tree

すべてKeyboard navigationとPointer interactionを考慮する。

Accessibilityを後付けにせず、widget semantic情報を持たせる。

---

# 28. Compositor / Window System

## 28.1 Window state

最低限:

- created
- mapped
- visible
- focused
- minimized
- maximized
- fullscreen
- closing
- destroyed

## 28.2 Rendering

Appは自分のsurfaceを描画し、Compositorが画面へ合成する。

## 28.3 Isolation

アプリが他アプリのsurfaceへ勝手に書き込めない。

---

# 29. Desktop Shell

SB Desktopには、一般ユーザーがOSを操作できるShellを用意する。

必要なもの:

- App launcher
- Task/window management
- Desktop area
- System status
- Notification center
- Power controls
- Network indicator
- Settings entry
- File Manager entry
- Terminal entry

デスクトップの見た目は軽量性を損なわない範囲で設計する。

---

# 30. 初回起動セットアップ

これはSBの重要なUX要件である。

## 30.1 画面

**GUIデスクトップが表示された後**に初回セットアップを表示する。

ターミナル画面ではない。

基本UI:

```text
Welcome to SuiraBox

Language
[ English ▼ ]

[ Continue ]
```

## 30.2 初期言語

- 日本語
- English
- 中文
- Español

## 30.3 処理

```text
Desktop ready
 ↓
Check first-run state
 ↓
No valid configuration?
 ↓
Show language popup
 ↓
User chooses language
 ↓
Validate
 ↓
Save atomically
 ↓
Activate locale
 ↓
Continue desktop
 ↓
Mark completed
```

## 30.4 キーボードレイアウト

言語とキーボード配列を同一視しない。

例えばEnglishを選んでも、物理キーボードが日本語配列ならJP layoutを選べる。

推奨初回順:

1. Language
2. Region/time zone
3. Keyboard layout
4. Network
5. Optional privacy/performance settings

ただし、最初の言語ポップアップ自体は軽く、小さくする。

ユーザー名/パスワード入力が必要な画面を作る場合は、キーボードレイアウトが確定してから行う。

## 30.5 設定破損

first-run markerが破損した場合:

- setup状態へ安全に戻す
- infinite loopを防ぐ
- 既存ユーザーデータを削除しない

---

# 31. Localization

ユーザー向け文字列をソースコードへ直接大量に埋め込まない。

## 31.1 Message ID

例:

```text
ui.welcome.title
ui.language.label
ui.continue
settings.display
error.network.timeout
panic.kernel.page_fault
```

## 31.2 Fallback

翻訳が存在しない場合はfallback languageへ戻す。

## 31.3 可変長

日本語から英語、英語からスペイン語など、文字列長が変わってもUIが壊れない設計にする。

---

# 32. Settings

設定アプリは「細かく設定できる」と「見やすい」を両立する。

カテゴリ例:

- System
- Display
- Appearance
- Sound
- Network
- Keyboard & Mouse
- Language & Region
- Storage
- Applications
- Accounts
- Privacy & Security
- Updates
- Performance
- Developer
- Recovery

## 32.1 詳細設定

初心者画面を壊さないため、Advanced/Developerへ分離できる項目は分離する。

## 32.2 検索

設定項目数が増えても探せるよう、Settings内検索を用意する。

## 32.3 Preview / Apply

変更によって再起動が必要な場合、適用前に明示する。

---

# 33. Accounts / Permissions

ユーザー・権限は後付けで作るのではなく、userspace APIとsecurity modelへ統合する。

必要な概念:

- User ID
- Group
- Credential
- Session
- Permission
- Privilege
- Admin/elevated operation

管理操作はGUIとCLIで同じsecurity policyを使用する。

---

# 34. Network Stack

```text
NIC driver
 ↓
Ethernet/link
 ↓
IPv4/IPv6
 ↓
ARP/ND
 ↓
Routing
 ↓
UDP/TCP
 ↓
DNS/DHCP
 ↓
Sockets
 ↓
Network Manager
```

## 34.1 必須機能

- Loopback
- Ethernet
- IPv4
- IPv6
- ARP/Neighbor Discovery
- Routing
- UDP
- TCP
- DNS
- DHCP
- Static configuration
- Socket API
- Firewall/policy later

## 34.2 Timeout

Network operationでGUIを固めない。すべての外部通信にtimeout、cancel、failure状態を持たせる。

---

# 35. Network Manager

GUIから、

- interface
- connected network
- address
- DNS
- route
- proxy later

を設定できる。

CLIでも同じ状態を確認・変更できる。

GUIとCLIの設定が競合しないよう、Network Managerをsingle ownerとする。

---

# 36. Terminal / Shell

SB Desktopには本物のTerminalを搭載する。

## 36.1 基本機能

- command execution
- environment
- stdin/stdout/stderr
- redirection
- pipe
- process control
- filesystem commands
- network tools
- package management
- diagnostics
- recovery commands

## 36.2 GUIとの関係

Terminal用APIとGUI用APIを別々に再実装しない。同じservice/permission/system APIを使用する。

---

# 37. Package Manager

SBの軽量性を成立させる重要なSubsystem。

```text
Repository metadata
 ↓
Resolve
 ↓
Download
 ↓
Verify
 ↓
Stage
 ↓
Transaction
 ↓
Install
 ↓
Register
```

## 37.1 Package identity

各packageは、

- name
- version
- architecture
- dependencies
- conflicts
- files
- permissions
- metadata

を持つ。

## 37.2 Integrity

少なくともchecksum/integrity validationを行う。

Official repositoryでは署名検証を行う。

## 37.3 CLI/GUI排他

CLIとSB Storeが同時にpackage transactionを実行しない。

共通のtransaction lockを持つ。

状態:

```text
idle
 ↓
locked
 ↓
transaction active
 ↓
commit
 ↓
unlock
```

GUI側でlock中の場合は「別のソフトウェア操作を処理しています」と表示し、勝手に同時変更しない。

---

# 38. SB Store

SB Storeはpackage managerのGUI frontendである。

## 38.1 機能

- Search
- Category
- Details
- Size
- Dependencies
- Install
- Remove
- Update
- Cancel download where safe
- Progress
- Error details

## 38.2 ベースOSを軽くする

最初から全部のアプリをISOへ入れない。

基本OSは必要最小限にし、Storeから追加する。

---

# 39. ダウンロード速度・データ量

速度だけを最適化して安全性を落とさない。

可能な手法:

- compressed metadata
- compressed package
- resumable download
- cache
- parallel transfer when beneficial
- mirror/CDN later
- delta update later

既にローカルに存在する正しいデータを再ダウンロードしない。

---

# 40. Driver Framework

DriverはKernel内部へ直接ベンダー依存を撒き散らさない。

共通Device Modelを作り、driverはinterfaceを実装する。

## 40.1 初期優先度

1. PCI
2. framebuffer/display
3. keyboard
4. mouse
5. storage
6. network
7. USB foundation
8. audio
9. GPU acceleration

## 40.2 GPU

段階的に、

```text
Generic framebuffer
 ↓
Basic hardware acceleration abstraction
 ↓
Intel
 ↓
AMD
 ↓
NVIDIA consumer/professional
```

などへ進める。

NVIDIA RTX系を含む高性能GPUは、PCI detectionだけでは「対応」としない。実際にdisplay accelerationや必要機能を動作検証できた時点で対応対象へ昇格させる。

---

# 41. USB

将来的なデスクトップ利用ではUSBは重要。

必要な層:

```text
USB Host Controller
 ↓
USB core
 ↓
USB device class
 ↓
Driver
```

Keyboard/mouseなど基本的入力をUSB経由でも扱えるようにする。

Hotplugを前提とし、device attach/detach eventを設ける。

---

# 42. Audio

将来的に、

- Device enumeration
- Playback
- Recording
- Volume/mute
- Per-app routing later

を実装する。

Audio failureでGUI全体を止めない。

---

# 43. Power Management

- Shutdown
- Reboot
- Sleep later
- Hibernate later
- AC/battery status
- Thermal status
- CPU power policy later

電源ボタンイベントはuserspace serviceとKernelで安全に処理する。

---

# 44. Clipboard / Notifications / Accessibility

## Clipboard

- copy
- paste
- clipboard owner
- data type later

## Notifications

- application notification
- system notification
- update notification
- error notification

## Accessibility

将来的に、

- keyboard-only navigation
- scalable UI
- high contrast
- screen reader hooks
- reduced motion

などへ拡張する。

---

# 45. Error UX / Support Report

重大な問題の画面には、必ず「修理・サポートに役立つ情報」を残す。

## 45.1 Support Report

含める候補:

- OS version
- build ID
- hardware summary
- driver summary
- kernel log
- service states
- storage health where safe
- network configuration summary without secrets
- error IDs
- crash metadata

除外:

- password
- authentication token
- private key
- session credential
- raw secret
- unnecessary private user content

ユーザーが自分で保存・共有できる形にする。

---

# 46. Crash Dump

Kernel crash時には可能な範囲でcrash dumpを保存する。

保存ができない場合は、画面へその事実を表示する。

Crash dump parserを将来的に開発者向けに提供する。

---

# 47. Recovery

目標は「全部止める」ことではなく「壊れた一部分だけ復旧する」こと。

例:

```text
App crash
 → restart app

Network service crash
 → restart network service

Display service crash
 → restart display stack if safe

Package transaction failure
 → rollback transaction

System boot failure
 → recovery mode

Kernel integrity failure
 → stop safely
```

Recovery failureが無限ループを作らない。

---

# 48. Update System

更新は、

```text
Check metadata
 ↓
Verify metadata
 ↓
Resolve dependencies
 ↓
Download
 ↓
Stage
 ↓
Verify
 ↓
Activate atomically
 ↓
Health check
 ↓
Commit
```

途中で電源断しても「完全に壊れた更新済み・更新前でもない半端な状態」を作らない。

必要なら前の状態へrollbackする。

---

# 49. Data Cleanup

「見えない要らないデータ」を減らすが、安全性を最優先する。

## 安全な候補

- expired temp
- obsolete package cache
- orphaned cache
- stale generated artifact
- expired diagnostic scratch data

## 自動削除禁止

- user documents
- unknown files
- active package data
- recovery data
- credential material

Cleanup engineにはdata classificationとownershipを持たせる。

---

# 50. Security Architecture

## 50.1 基本

- Kernel/userspace isolation
- Memory protection
- Syscall validation
- Permission checks
- Package verification
- Update verification
- Least privilege
- Driver isolation where practical
- Sandboxing where practical

## 50.2 Supply chain

SB official repositoryでは将来的に、

- Signed metadata
- Signed packages
- Trusted keys
- Key rotation
- Revocation
- Repository pinning/policy

を扱う。

---

# 51. Application isolation

すべてのアプリを同じ権限で動かさない。

段階的に、

- normal user process
- restricted app
- sandboxed app
- privileged/system service

を区別する。

UIだけで「安全そう」に見せず、Kernel/userspace boundaryで実際に制御する。

---

# 52. Time / Date / Region

OS全体で共通のtime serviceを持つ。

- monotonic time
- wall clock
- RTC
- time zone
- locale
- daylight saving data as applicable

Scheduler timeoutとユーザー表示時刻を同一概念にしない。

---

# 53. Performance Architecture

軽量化は「削ったファイル数」ではなく測定値で評価する。

最低限測定:

- boot time
- idle RAM
- idle CPU
- background wakeups
- storage footprint
- GUI frame latency
- app launch latency
- network overhead
- package download size
- update size

## 53.1 Lazy loading

不要なサービス・フォント・アプリ・driverは必要になるまでロードしない。

## 53.2 No pointless polling

イベント駆動を優先する。

## 53.3 Memory reuse

同じデータを複数のサービスが重複保持しない。

---

# 54. Minecraftとの関係

SBは汎用Desktop OSを維持しながら、Minecraft利用を重要なユースケースの一つとして扱う。

将来的に、

- JVM runtime optimization
- Minecraft instance manager
- Minecraft Server manager
- storage optimization
- network policy
- scheduling/runtime hints
- benchmark suite

などを追加できる。

ただしMinecraft機能をKernelへ直接埋め込み、汎用OSとしての設計を壊さない。

Minecraft向け機能はPolicy / Runtime / Application層に置く。

---

# 55. Build System

## 55.1 必須

- reproducible-ish build process
- explicit architecture
- freestanding kernel
- controlled linker
- ISO creation
- checks
- QEMU boot test

## 55.2 Assembly errors

32-bit assemblerへ64-bit relocationを誤って渡す事故を防ぐ。

Architectureごとに、

- operand width
- relocation type
- symbol addressability
- code model

を明示する。

---

# 56. CI/CD

基本CI:

```text
Checkout
 ↓
Install build dependencies
 ↓
Build
 ↓
Validate boot image
 ↓
Inspect layout
 ↓
QEMU boot
 ↓
Serial log assertions
 ↓
Upload artifacts
```

将来追加:

- unit tests
- integration tests
- package tests
- GUI startup test
- first-run persistence test
- storage tests
- network tests
- crash tests
- recovery tests
- performance regression tests

## 56.1 CIの原則

失敗を隠してgreenにしない。

Timeoutを伸ばすだけで問題を消さない。

Expected outputを削除して成功扱いにしない。

---

# 57. QEMU / Virtual Hardware Test

QEMUは最初の自動検証環境。

最低限:

- x86_64 boot
- ISO boot
- Serial output
- memory tests
- VMM tests
- scheduler tests
- userspace preparation
- timer

将来:

- disk image
- network device
- framebuffer
- multiple CPUs
- USB
- UEFI boot

---

# 58. 実機検証

QEMUで通ったことをHardware compatibilityと同一視しない。

実機では少なくとも、

- Boot
- Storage
- Keyboard/mouse
- Display
- Network
- Power
- Reboot/shutdown

を確認する。

さらにベンダー別GPU等を追加していく。

---

# 59. Test design

Subsystemごとに、

1. Happy path
2. Boundary
3. Invalid input
4. Resource exhaustion
5. Concurrent access
6. Recovery
7. Persistence
8. Performance

を確認する。

## 59.1 例: PMM

- first allocation
- repeated allocation
- free/reallocate
- invalid alignment
- invalid range
- reserved region
- exhaustion
- double free
- bootstrap/real map transition

## 59.2 例: Package Manager

- valid install
- missing dependency
- conflict
- corrupt download
- interrupted download
- transaction lock
- rollback
- GUI/CLI concurrent request

## 59.3 例: First Boot

- no config
- valid config
- invalid config
- language save
- reboot persistence
- keyboard layout mismatch
- translation fallback

---

# 60. Public Project / Official Ecosystem

SBはコードだけでなく、世界中から参加できるプロジェクトとして公開する。

## 60.1 Official Website

資金をかけない前提で、GitHub Pagesを優先する。

掲載予定:

- SB紹介
- Features
- Downloads
- Documentation
- Development status
- Hardware compatibility
- Roadmap
- Security advisories
- Release notes
- Contributing
- Community links

## 60.2 Official social/community

Suiramをプロジェクト/コミュニティの識別名とする。

公式SNSやDiscordなどは、なりすまし対策として公式サイトから導線を示す。

---

# 61. Documentation architecture

`SB_OS_DESIGN.md`は最上位。

個別ドキュメントは詳細実装を補足する。

例:

```text
SB_OS_DESIGN.md
 ↓
docs/BOOT.md
docs/MEMORY.md
docs/HEAP.md
docs/SCHEDULER.md
docs/SYSCALL_ABI.md
docs/FILESYSTEM.md
docs/DRIVERS.md
docs/GUI_DESKTOP_PLAN.md
docs/FIRST_BOOT_SETUP.md
docs/SB_SETTINGS.md
docs/SB_STORE.md
...
```

個別文書とこのマスターが矛盾した場合、マスターの更新を検討し、設計変更を明示する。

---

# 62. Repository Structure

目標構造:

```text
boot/
kernel/
userspace/
docs/
tests/
site/
.github/
Makefile
linker.ld
README.md
SB_OS_DESIGN.md
```

Kernel目標:

```text
kernel/
  arch/x86_64/
    boot/
    cpu/
    interrupt/
    paging/
    timer/
    acpi/
    smp/
  mm/
    pmm/
    vmm/
    heap/
  sched/
  process/
  ipc/
  syscall/
  fs/
  net/
  time/
  power/
  security/
  log/
  panic/
  drivers/
```

Userspace目標:

```text
userspace/
  init/
  services/
  gui/
  apps/
```

これは「空ディレクトリを作れ」という意味ではない。実装されたSubsystemに合わせて育てる。

---

# 63. 実装の順番

以下の順番を基本とする。依存関係上必要なら前後させるが、上位機能を下位基盤未完成のまま「完成扱い」しない。

## Phase 0 — Build

Toolchain → Build → Link → ISO → QEMU → CI

## Phase 1 — Boot

Bootloader → CPU state → stack → boot info → kernel layout

## Phase 2 — Memory

PMM → VMM → Heap → memory diagnostics

## Phase 3 — CPU runtime

GDT → TSS → IDT → Exceptions → IRQ → Timer → synchronization

## Phase 4 — Execution

Scheduler → Thread → Process → IPC → Syscall → ELF → Userspace

## Phase 5 — Storage

Block → Partition → Filesystem → VFS → Configuration

## Phase 6 — Platform

PCI → ACPI → SMP groundwork → Input → Display → USB → Network → Audio → Power

## Phase 7 — GUI

Framebuffer → font/text → event → compositor → windows → toolkit → shell

## Phase 8 — First run

Config service → Localization → Language popup → Keyboard layout → Settings

## Phase 9 — Network services

Network Manager → DNS/DHCP → Firewall/policy

## Phase 10 — Software ecosystem

Package Manager → SB Store → Update system

## Phase 11 — Reliability

Logging → diagnostics → crash dump → recovery → rollback

## Phase 12 — Security

Permissions → privilege separation → package/update signing → sandbox boundaries

## Phase 13 — Optimization

Boot → RAM → CPU → storage → graphics → network → application launch

## Phase 14 — Release

RC → complete test matrix → documentation → signatures/checksums → release

---

# 64. Project state model

Every component uses one of the following states:

```text
PLANNED
 ↓
DESIGNED
 ↓
SKELETON
 ↓
PARTIAL
 ↓
FUNCTIONAL
 ↓
BUILD-VERIFIED
 ↓
RUNTIME-VERIFIED
 ↓
HARDWARE-VERIFIED
 ↓
RELEASE-READY
```

`FUNCTIONAL` means the feature actually works in its supported scenario.

`RUNTIME-VERIFIED` means automated or controlled runtime testing confirmed it.

`HARDWARE-VERIFIED` means real hardware testing confirmed the declared support level.

---

# 65. Current implementation state

**この章は必ず実際のコードとCIで更新する。**

現時点の重要な事実:

```text
Build/ISO                         ✅
Multiboot validation              ✅
Kernel entry                      ✅
PCI enumeration                   ✅
PMM initialization                ❌ current blocker
VMM                               not yet runtime-verified beyond PMM
Heap                              not yet runtime-verified beyond PMM
GDT/TSS                           not yet reached in current QEMU path
Scheduler                         not yet reached in current QEMU path
Process/Syscall                   not yet reached in current QEMU path
Userspace                         not yet reached in current QEMU path
Display                            not yet reached
GUI                               not yet reached
First Boot Language UI            not yet reached
Settings                          not yet reached
Network                           not yet reached
Package Manager                   not yet reached
SB Store                          not yet reached
Recovery                          not yet reached
Release                           not yet reached
```

現在のボトルネックはPMMである。QEMU smoke testは`Memory: PMM init begin`の後に停止しているため、次の作業はPMMの実行時停止を根本原因から解決することである。

未到達のSubsystemを「ソースファイルが存在する」という理由だけで完成扱いしない。

---

# 66. PMM問題の復旧手順

現在地点からのデバッグ方法:

1. `kernel/mm/pmm.c`を読む。
2. PMMのstatic storageの配置を確認する。
3. Linker map / `nm`でbitmap/page_count/free_countのアドレスを確認する。
4. Boot stack、page tables、kernel imageと重複していないか確認する。
5. `pmm_init()`を、reset / range setup / count recomputeの単位で検証する。
6. 各段階の境界ログは、allocator/lockに依存しない最小Loggerを使用する。
7. QEMUで最小ケースを実行する。
8. Boot informationがPMMに踏まれていないか確認する。
9. BIOS/Multibootで受け取った情報のlifespanを確認する。
10. 正常化したら、まずPMM単体を検証し、その後VMMへ進む。

この問題を雑に「時間を増やす」「testを消す」で解決しない。

---

# 67. Complete End-to-End State Transitions

## 67.1 Boot

```text
OFF
 ↓
FIRMWARE
 ↓
BOOTLOADER
 ↓
KERNEL_ENTRY
 ↓
EARLY_CPU_READY
 ↓
MEMORY_READY
 ↓
INTERRUPTS_READY
 ↓
SCHEDULER_READY
 ↓
USERSPACE_READY
 ↓
DISPLAY_READY
 ↓
DESKTOP_READY
```

## 67.2 First-run

```text
DESKTOP_READY
 ↓
NO_VALID_CONFIG
 ↓
LANGUAGE_DIALOG
 ↓
LANGUAGE_SELECTED
 ↓
CONFIG_VALIDATE
 ↓
CONFIG_COMMIT
 ↓
LOCALE_APPLY
 ↓
FIRST_RUN_COMPLETE
 ↓
NORMAL_DESKTOP
```

## 67.3 Package install

```text
REQUESTED
 ↓
RESOLVING
 ↓
LOCKED
 ↓
DOWNLOADING
 ↓
VERIFYING
 ↓
STAGING
 ↓
INSTALLING
 ↓
HEALTH_CHECK
 ↓
COMMITTED
 ↓
UNLOCKED
```

Failure:

```text
ANY_TRANSACTION_STATE
 ↓
FAILURE
 ↓
ROLLBACK
 ↓
RESTORE_KNOWN_STATE
 ↓
UNLOCK
 ↓
REPORT
```

---

# 68. Data Ownership Model

重要データには必ずowner subsystemを定める。

例:

```text
Language config         → Configuration/Locale service
Network config          → Network Manager
Package database        → Package Manager
User account data       → Account service
Window state            → Window system / Desktop Shell
Driver state            → Driver subsystem
Kernel logs             → Logging subsystem
Crash report            → Diagnostic subsystem
```

別Subsystemが同じ設定を独自コピーして、互いに別の値になることを避ける。

---

# 69. Persistent Data Versioning

ユーザーデータ、system configuration、package databaseなどを更新する場合はversionを持たせる。

破壊的変更:

```text
old format
 ↓
migration validation
 ↓
new format
 ↓
verification
 ↓
commit
```

Migration失敗時は旧形式を保護する。

---

# 70. Privacy

SBは「取れるから取る」という設計をしない。

Telemetryを追加する場合、目的・データ・保存期間・共有先・無効化方法を明示する。

Diagnosticsでは必要最低限のみ収集する。

---

# 71. Developer Experience

AIや人間の開発者が同じリポジトリを扱えるようにする。

目標:

```text
Clone/CodeSpace
 ↓
Install documented dependencies
 ↓
make
 ↓
QEMU
 ↓
Tests
```

開発環境自体も将来的に`.devcontainer`などで再現可能にする。

---

# 72. AI Handoff

別AIへ引き継ぐ場合、最初にこのファイルを読ませる。

その後、必ず:

1. Repository tree確認
2. Current source確認
3. Current CI確認
4. Relevant docs確認
5. Current state判定
6. Smallest unfinished prerequisite特定
7. Implementation
8. Build
9. Runtime test
10. CI
11. Failure analysis
12. Documentation update

を行う。

AIが知らない歴史的事情を勝手に補完しない。仕様上不明な点を発見した場合は、既存の設計と互換性を最優先にして記録する。

---

# 73. What not to do

- GUIだけ作ってKernelが壊れたまま進めない。
- CIが赤いのに「完成」と言わない。
- Hardware supportを未検証で宣言しない。
- PMMを無視してVMMのモックを作らない。
- GUIエラーでBSODを乱発しない。
- RSODを普通のエラー画面として使用しない。
- ユーザーのファイルをcleanupという名目で勝手に削除しない。
- CLIとGUIで同じデータベースを別々に操作しない。
- LoggingがKernelのallocator/lockを再帰的に呼び出す設計を作らない。
- Boot情報を使用中にfree扱いしない。
- 機能追加のためだけに常駐サービスを増やさない。
- 未使用ライブラリやデータをBase ISOへ無制限に詰め込まない。
- 一時的なdiagnostic codeを恒久化しない。
- 下位層へ上位層の概念を持ち込まない。

---

# 74. Release Engineering

Release前に以下を実行する。

```text
Source freeze
 ↓
Build clean
 ↓
Full CI
 ↓
QEMU matrix
 ↓
First-boot test
 ↓
Persistence test
 ↓
Package test
 ↓
Recovery test
 ↓
Hardware smoke test
 ↓
Security review
 ↓
Artifact verification
 ↓
Checksums/signatures
 ↓
Release notes
 ↓
Website publication
```

## 74.1 Release artifact

必要に応じて:

- ISO
- checksum
- signature
- debug symbols where intended
- release notes
- installation guide
- recovery guide
- compatibility list

を公開する。

---

# 75. 完成条件 — SB Desktop v1

次をすべて満たすことを目標とする。

## Boot

- Supported x86_64 machine/QEMUから起動できる。
- BootloaderからKernelへ正しく移行する。
- メモリ領域を安全に扱う。
- Kernel panicを除き、不要な停止がない。

## Core

- PMM
- VMM
- Heap
- Interrupt
- Timer
- Scheduler
- Process
- Syscall
- Userspace

がRuntime-verifiedである。

## Desktop

- GUI起動
- display
- keyboard
- mouse
- windows
- compositor
- desktop shell

が機能する。

## First Boot

- Desktop到達後に言語popupが出る。
- select boxで言語を選べる。
- Japanese / English / Chinese / Spanishが使用できる。
- 設定が保存される。
- 再起動後も維持される。
- Keyboard layoutを別に設定できる。

## Daily Use

- Files
- Settings
- Network
- Terminal
- Package Manager
- SB Store

が機能する。

## Reliability

- Normal errors are understandable.
- BSOD is reserved for kernel/system emergencies.
- RSOD is extremely rare.
- Support Report exists.
- Recovery exists.
- Update rollback exists where supported.

## Performance

- 不要な常駐処理を最小化
- Base imageを最小化
- Background wakeupsを測定・削減
- 起動時間を測定
- Idle RAMを測定

## Security

- userspace/kernel isolation
- package verification
- update verification
- permission enforcement
- secret exclusion from diagnostics

を実装・検証する。

---

# 76. 将来のSB

v1完成後は、共通基盤を壊さずに枝分かれする。

```text
                 SB Desktop Core
                       │
        ┌──────────────┼──────────────┐
        ↓              ↓              ↓
     Gaming       Workstation      General
        │              │
        ↓              ↓
  GPU/JVM tuning   Pro hardware
        │
        └──────────────┬──────────────┘
                       ↓
                 Server / High-end
                       ↓
               Data Center later
```

高性能GPU、サーバー、データセンター向け機能などは、**最初のGUI Desktopを完成させた後に拡張する**。

---

# 77. 最終的な設計思想

SBは「何でも最初から入っている巨大OS」を目指さない。

SBが目指すのは、

```text
小さい基盤
+ 明確な構造
+ 必要な機能だけ追加
+ 細かい設定
+ 使いやすいGUI
+ 完全なCLI
+ 強い診断
+ 安全な復旧
+ 測定可能な性能
+ 拡張可能なHardware abstraction
```

を同時に成立させたOSである。

ユーザーがPCの性能を活かせるよう、OS自身が余計な負荷を抱え込まない。

しかし「軽い」という理由で機能や安全性を削るのではなく、**不要なものを外へ追い出し、必要なものは正しく作る**。

---

# 78. このファイル自体の役割

このファイルは単なる説明書ではない。

**SBの設計・目的・完成形・実装順・Subsystem・データ・状態・UX・障害対応・運用・リリース・将来方針を保存するプロジェクトバックアップである。**

会話履歴が失われても、このファイルを読めば、少なくともSBの意図と構造を再構成できなければならない。

ただし実装状態については常にコードとCIを優先する。

```text
SB_OS_DESIGN.md
      ↓
プロジェクトの意図・設計・完成形
      ↓
個別Subsystem docs
      ↓
Source code
      ↓
Build / CI / QEMU / Hardware
      ↓
Verified implementation
```

**この設計書を読んで初めてSBを理解したことになる。**

重要な設計変更を行った場合、このファイルを更新し、古い設計との違いを追跡可能にする。

---

# 79. Master Checklist

```text
[ ] Build foundation
[ ] Bootloader
[ ] Firmware abstraction
[ ] Boot memory ownership
[ ] PMM
[ ] VMM
[ ] Heap
[ ] GDT
[ ] TSS
[ ] IDT
[ ] Exceptions
[ ] IRQ
[ ] Timer
[ ] ACPI
[ ] SMP
[ ] Synchronization
[ ] Scheduler
[ ] Thread
[ ] Process
[ ] IPC
[ ] Syscall ABI
[ ] ELF loader
[ ] Userspace init
[ ] Service manager
[ ] Block layer
[ ] Partition
[ ] Filesystem
[ ] VFS
[ ] Configuration
[ ] Account/permission
[ ] PCI
[ ] USB
[ ] Input
[ ] Display
[ ] GPU abstraction
[ ] Audio
[ ] Power
[ ] Unicode
[ ] Fonts
[ ] IME framework
[ ] Event system
[ ] GUI toolkit
[ ] Compositor
[ ] Window system
[ ] Desktop shell
[ ] First-run language UI
[ ] Keyboard layout UI
[ ] Localization
[ ] Settings
[ ] Network stack
[ ] Network Manager
[ ] Terminal
[ ] Package Manager
[ ] SB Store
[ ] Logging
[ ] Crash dump
[ ] Error UX
[ ] BSOD
[ ] RSOD
[ ] Recovery
[ ] Update
[ ] Rollback
[ ] Cleanup engine
[ ] Security model
[ ] Package signing
[ ] Update signing
[ ] Performance suite
[ ] Hardware matrix
[ ] QEMU matrix
[ ] Real hardware testing
[ ] Release pipeline
[ ] Website
[ ] Documentation
[ ] Public ecosystem
[ ] SB Desktop v1 release
```

---

# 80. Final rule

**最優先するのは「SBらしい、実際に動くOSを完成させること」。**

設計の美しさだけでも、巨大な機能一覧だけでも完成ではない。

```text
正しい設計
+ 正しい実装
+ 実際のテスト
+ 実際のユーザー体験
+ 実際の復旧能力
= SB Desktop
```

現在はPMMを突破することから進める。