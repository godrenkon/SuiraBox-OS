# SuiraBox OS External Resource Architecture

## 目的

SuiraBox OS の ISO / kernel / 基本 desktop をできるだけ小さく保ちつつ、初回起動後に大量の追加ダウンロードを要求しない。OSとして日常的に必要な基本機能は最初から完成した状態で利用でき、利用者が選ぶ大容量データや追加機能だけを外部 Resource Repository から必要時に取得する。

Resource は次の3層に分ける。

- **Core / Built-in**: OSとして必要、または全ユーザーが高確率で使う基本機能。ISOに最初から含め、オフラインでも使える。
- **Optional / Local**: 必須ではないが、設定でON/OFFできる機能。小容量ならOSへ同梱可能な独立モジュールとして保持し、無効時はロードしない。
- **Remote / On-demand**: 大容量、利用者ごとに選択が分かれる、または更新頻度が高いデータ・追加アプリ。外部Repositoryから選択時だけ取得する。

## Coreの原則

Coreを削りすぎて「起動したのに何もできないOS」にしない。少なくとも以下は最初から完成状態で提供する。

- kernel / boot / memory / process / VM / syscall
- 必須hardware support
- display / keyboard / mouse
- filesystem / configuration / recovery
- compositor / window manager / Desktop Shell
- Settings
- Terminal / CLI
- File Managerの基本機能
- 最小の標準フォント / glyph
- fallback theme
- fallback wallpaper / background
- resource manager / cache / integrity verification
- Resource未取得時のfallback UI

これらはRemote取得を前提にしてはいけない。ネットワークが存在しない環境でも、起動、設定変更、ファイル操作、Terminal利用、復旧が成立する必要がある。

## Optional / Local

Optionalは「無効にしてもOSが正常に使える」機能だけを対象とする。

例:

- 追加言語の軽量pack
- 追加テーマ
- 追加フォント
- アクセシビリティ拡張
- 小型ユーティリティ
- 追加shell provider

設定画面から有効/無効を変更できる。無効時は可能な限り対応コード・データをロードせず、RAM使用量を増やさない。

## Remote / On-demand

Remoteは「利用者が選ぶ」「容量が大きい」「頻繁に更新される」のいずれかを満たすものを優先する。

例:

- 全言語の大規模locale pack
- 高解像度 wallpaper / wallpaper collection
- 大型theme / icon pack
- notification / sound pack
- 追加アプリ / app bundle
- 大規模help / documentation data
- Minecraft関連の追加resource
- 開発toolchain
- 大規模サンプルデータ

Remoteは選択されていない限り取得しない。OS本体へ大量の未使用assetを埋め込まない。

## Resource Repository

予定Repository:

`godrenkon/SuiraBox-OS-Resources`

現在のGitHub接続では新規repository作成mutationが提供されていないため、このRepository自体はまだ自動作成していない。OS側の契約を先に固定し、Repository作成後にendpoint設定だけで接続できるようにする。

## Manifest契約

各resource entryは少なくとも以下を持つ。

- `id`: stable logical ID
- `version`: resource version
- `type`: resource type
- `size`: payload size
- `sha256`: immutable content hash
- `path`: repository relative path
- `compression`: supported compression
- `min_os`: required SB Resource ABI version
- `dependencies`: required resource IDs

OSはRepository URLを各assetへ直接ハードコードせず、信頼済みmanifestからpathを解決する。

## Content-Addressed Cache

取得済みresourceはcontent hashを主キーとして共有キャッシュする。

```text
/cache/suirabox/objects/sha256/<first2>/<remaining62>
```

同一payloadを複数機能が参照しても1個だけ保持する。inactive objectは空き容量が必要なときにGCし、通常時のI/Oを増やさない。ユーザーがpinしたresourceはGC対象外にする。

## Atomic Install

1. payload sizeと保存領域を検査
2. temporary objectを作成
3. streaming download
4. 受信中にSHA-256を計算
5. size / hash / version / ABIを検証
6. 必要なら展開前後のサイズ制限を検証
7. content-addressed objectへatomic activation
8. active stateをcommit

失敗したtemporary objectは削除し、以前のactive resourceはそのまま維持する。

## ダウンロード量削減

優先順:

1. verified local cache hit
2. range / delta update
3. full payload

ユーザーが使っていないresourceを勝手に更新しない。manifestだけ更新が必要な場合もmanifestを軽量に保つ。

## 言語システム

Coreにはfirst bootと基本UIを表示できる最小glyph / fallback文字列を残す。通常UIの大量翻訳データはlocale resourceへ分離する。

日本語を選択した場合は日本語packだけを取得し、他言語packを取得しない。すでにcache済みなら再取得しない。locale packが取得できなくてもfallback UIへ戻れる。

## テーマ・壁紙

Coreには軽量なfallback themeと最小backgroundを保持する。大容量theme、icon、壁紙collectionはRemote。

テーマはdata-onlyとして扱い、OS binaryへコードを埋め込まない。テーマ変更時にkernelを再構築する必要はない。

## アプリシステム

Settings / Terminal / 基本File Managerなど、OSの基本操作に必要なアプリはCoreとして最初から利用できる。

一方、追加アプリはOptionalまたはRemoteとし、ユーザーが必要なものだけ有効化/取得する。Remote appが未取得でもCore Desktopは正常に起動できる。

## セキュリティ

External resourceは未検証のまま有効化しない。

- manifest version / syntax validation
- SHA-256 verification
- package size limit
- path traversal rejection
- decompression bomb limit
- integer overflow checks
- dependency cycle detection
- OS ABI compatibility checks
- 将来のsigned manifest / release key validation

Remote resourceはkernel置換機構ではなく、dataまたは通常のuserspace packageとして扱う。

## Offline behavior

ネットワークなしでもCoreは完全に使用できる。既にcacheされたOptional / Remote resourceも継続利用できる。

未取得resourceを選択した場合は「ダウンロードできない＝OSが使えない」にしない。設定値を保持し、resource unavailableとして扱い、接続回復後に取得可能にする。

## OSサイズ最小化ルール

OS本体から外へ出す優先度が高いもの:

- 全言語の大量翻訳データ
- 大量font pack
- 高解像度wallpaper
- 大型theme / icon pack
- sound pack
- tutorial / documentation bulk data
- large sample data
- optional applications
- Minecraft等のfeature-specific resources

OS本体へ残すもの:

- boot / kernel
- process / VM / syscall
- storage / filesystem / recovery
- 必須hardware support
- display / input
- compositor / window manager / Desktop Shell
- Settings / Terminal / basic File Manager
- minimal font / glyph / fallback theme
- resource manager and integrity verifier
- offline fallback UI

## 基本UX原則

ユーザーは「OSをインストールしたのに、次に何十個も機能をダウンロードしないと使えない」状態を経験しない。

初回起動直後から基本Desktopは完成している。追加データを選択した時だけ、その機能に必要な最小resourceを取得する。

また、設定画面からOptional featureをOFFにした場合は、必要なら後からONへ戻せる。Remote resourceを削除してもCoreには影響しない。
