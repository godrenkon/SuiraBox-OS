# SuiraBox OS External Resource Architecture

## 目的

SuiraBox OS の ISO / kernel / 基本 desktop をできるだけ小さく保ち、言語データ、テーマ、壁紙、追加アプリ、アイコン、フォントなどの非必須データを OS 本体から分離する。

基本原則:

- OS 本体には起動に必須なコードだけを入れる。
- 非必須リソースは外部 Resource Repository からオンデマンド取得する。
- 取得対象は選択されたリソースだけにする。
- 同一リソースは SHA-256 の content ID でキャッシュし、再取得しない。
- 一時ファイルへ保存して検証完了後に atomic activation する。
- ダウンロード失敗時でも OS の既存状態を壊さない。
- Resource Repository の最新版を盲目的に信用せず、OS が許可した manifest / key policy に従う。
- ISO に巨大な fallback asset bundle を埋め込まない。起動維持に必要な最小 UI だけはコードとして保持する。

## Resource Repository

予定リポジトリ名:

`godrenkon/SuiraBox-OS-Resources`

現在の GitHub 接続では新規 repository 作成 mutation が提供されていないため、このリポジトリ自体はまだ自動作成していない。OS 側の契約は先に固定し、repository 作成後は endpoint を変更せず接続できる構成にする。

想定構成:

```text
manifest/
  manifest-v1.json
  channels/stable.json
locales/
  ja-JP/
    locale.pack.zst
  en-US/
    locale.pack.zst
  zh-CN/
    locale.pack.zst
  es-ES/
    locale.pack.zst
themes/
  default/
    theme.json.zst
    icons.pack.zst
wallpapers/
  default/
    wallpaper.avif
apps/
  terminal/
    app.sbx.zst
  files/
    app.sbx.zst
```

個別ファイルをそのまま参照するのではなく、manifest に immutable asset metadata を記載する。

## Manifest 契約

各 resource entry は少なくとも以下を持つ。

- `id`: 論理 ID
- `version`: リソース版
- `type`: `locale`, `theme`, `wallpaper`, `icon`, `font`, `app` など
- `size`: 展開前ではなく取得 payload のバイト数
- `sha256`: payload の content hash
- `path`: repository 上の相対パス
- `compression`: `none` または対応圧縮方式
- `min_os`: 必要 OS API version
- `dependencies`: 必要リソース ID の配列

OS は URL を固定文字列として各ファイルへ埋め込まず、信頼済み manifest から path を解決する。

## ダウンロード単位

`language` を Japanese に変更した場合、OS は `ja-JP` locale pack とその必須依存だけを取得する。

テーマ変更時は theme metadata、必要 icon pack、必要 wallpaper のみを取得する。

アプリ追加時はその app package と依存ライブラリだけを取得する。

これにより、例えば日本語ユーザーがスペイン語、別テーマ、未使用壁紙、未使用アプリのデータを持つ必要はない。

## Content-Addressed Cache

キャッシュは論理 ID ではなく content hash を主キーにする。

```text
/cache/suirabox/objects/sha256/<first2>/<remaining62>
```

同一 payload が複数リソースから参照されても 1 個だけ保存する。バージョン更新で payload が変わった場合は別 hash object として保存する。

active manifest が参照しなくなった object は GC 対象にする。GC は空き容量が必要になった時だけ実行し、通常時の I/O を増やさない。

## Atomic Install

1. 空き容量と payload size を確認。
2. 一時 object を作成。
3. streaming download。
4. 受信中に SHA-256 を更新。
5. byte count と hash を manifest と照合。
6. 圧縮 payload なら展開結果のサイズと必要な検証を行う。
7. object を content-addressed cache へ atomic rename。
8. active state を commit。

検証に失敗した一時 object は即時削除する。

## 再開と通信量削減

ネットワーク層が Range request を提供できる場合は部分取得を許可する。再開可能な temporary object に既取得 byte count を保持し、接続断で最初から再取得しない。

manifest 自体は小さく保ち、OS が保持する前回 manifest hash と同一なら再取得を省略する。

## 更新

更新は以下の優先順で行う。

1. local cache hit
2. delta / range update
3. full payload

未使用 resource は自動更新しない。

## 言語システム

OS core が保持するのは言語 ID と最小限の UI 記号だけにする。言語名、desktop menu、dialog、エラー文、設定画面などの全文文字列は locale pack に移す。

first boot は言語 pack が存在しなくても動作するよう、選択対象を固定 enum として扱える最小 UI を維持する。言語決定後に該当 locale pack を取得し、成功した場合のみ通常 UI の翻訳テーブルを有効化する。

## テーマシステム

色、spacing、window chrome、icon、cursor、wallpaper は theme resource へ分離する。ただし boot emergency UI の最小配色だけは kernel / desktop core に残す。

テーマはコードを含まず data-only にする。これによりテーマ追加のため OS binary を再ビルドしない。

## アプリシステム

標準アプリも将来的には OS core から分離する。kernel は process / VM / syscall / storage / network などの基盤だけを持ち、Terminal、File Manager、Settings などは application package として取得する。

ただし設定 UI など recovery に必要な最小機能は、ネットワーク障害時の復旧経路として別の minimal system app にできるようにする。

## セキュリティ

外部 resource は未検証のまま有効化しない。

manifest の信頼根は OS release に紐付け、resource repository のファイル URL が書き換えられても SHA-256 mismatch で拒否する。将来は release signing key による manifest signature を必須化する。

圧縮 bomb、整数 overflow、size mismatch、依存循環、path traversal、manifest version incompatibility を拒否する。

## オフライン時

オンライン取得できなくても、既に cache にある resource は利用可能にする。

未取得 resource を選択した場合は設定値を壊さず、resource unavailable 状態として扱う。次回接続時に再取得できる。

## OS サイズ削減方針

OS 本体へ原則として入れないもの:

- 全言語の翻訳文字列
- 大量のフォント
- 壁紙
- テーマ追加データ
- アイコン pack
- 標準アプリ本体
- サウンド pack
- チュートリアル / ヘルプ文書
- 大容量サンプルデータ
- 互換用の複数バージョン resource

OS 本体へ残すもの:

- bootloader integration
- kernel
- process / VM / syscall 基盤
- storage / filesystem 基盤
- 必須 hardware support
- 最小 desktop bootstrap
- 最小 recovery UI
- resource manifest client
- resource cache / integrity contract

この分離を前提に、将来的には通常の desktop ISO から標準アプリを外し、network-connected first boot で必要なものだけを取得する。
