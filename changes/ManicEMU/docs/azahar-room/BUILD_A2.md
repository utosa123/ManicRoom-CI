# 段階A2: 手動CIと独立ビルド

この版はビルド準備済みのソース/パッチであり、ビルド成功済みのiOS版ではない。Windows試験、静的workflow検査だけ実行済み。macOS実行、署名、インストール、PC実機Room、ゲームは未実行。

## 3種類のappと独立core

| 経路 | source/core | 目的・表示 |
|---|---|---|
| A baseline | 固定Manic/Libretroの未変更checkout＋元framework | 元の構成のunsigned archive。表示Manic Baseline |
| B ui | 今回のManic/Libretro patch＋元framework（SHA256固定検査） | Swift/ObjC compileと未対応時の動作。表示Manic UI・Room未対応、build-mode=ui |
| C experimental | 今回patch＋固定公開候補からの別framework | Roomへの接続試行準備。表示Manic Room 実験用。専用保存先、cheat/state/時間操作制限 |

build-app.shがarchive、package-unsigned.shがIPAを生成する。Bのbuild-ui-only.shはRoom symbolを要求しない。Cのbuild-unsigned-ipa.shはBへ迂回せず、別の元binaryをMANIC_ORIGINAL_COREで要求し、全てのRoom/独自契約検査を行う。通常置換用の旧check-core-exports.shは変更しておらず、候補は3 symbol不足で失敗する。

元frameworkを手元で上書きしない。Cはci/install-experimental.pyが使い捨てsrc/ManicEMU内で元frameworkをoriginal/へ退避してからコピー。自分の保存データの移動・自動移行なし。将来署名時も元app ID・App Group・iCloud containerを流用しない。今回は署名secretも新規署名toolも不要。

## GitHub Actionsの準備

成果物ルートは**専用CIハーネス**として配置する構成。.github/workflows/room-experiment.yml、.github/actionlint.yaml、ci/、changes/、patches/、provenance.jsonを同じrepositoryのルートへ置く。Manic本体のルートへworkflowだけをコピーしても動かない。

workflow_dispatchのみ。target=windows（初期値）/baseline/ui/core/allを選べる。allのCはWindows試験、candidate build、A/B両方の成功が必要。公開lobby/chat/host実装・YW2 workaroundは含まない。

ジョブ/stepを分離: Windows real ENet transport試験、A/B archive、公開core iOS arm64、iOS arm64 keyboard layout compile、custom+Room export検査、C archive、unsigned IPA。失敗時もprepare/build/configure/SPM/testログをupload（7日保存）。binary artifactも7日保存。自動push/release/署名/deployなし。

全sourceはcommit固定、submoduleはそのgitlink、SPMは既存Package.resolvedを強制して変更をcmpで拒否。patchは適用前git apply --check。Actionsは実在確認したcommit固定（logs/actions-pins.json）。macOSはmacos-26、Xcode 26.6の明示path、iphoneos SDKを記録、CMake 3.31.6をvenvへ固定。SDK・runner image・Python・compilerは実行時バージョンをログ化する。hosted image自体はimmutableではなく、Xcode pathが消えた場合は黙って代替しない。

公式runner一覧にはmacos-26 arm64、Xcode 26.6、iPhoneOS 26.5 SDKがある。標準runnerは3 CPU/7 GB RAM/14 GB SSD。並列core buildを2に制限。現checkoutの未取得LFS pointerは337ファイル、合計532,825,241 bytes（元Azahar取得分等は別）。依存/SPM/build/archiveは別に容量を消費するので、prepare.pyはLFS二重保存＋8 GiBを最低余裕として見積もり、容量不足で停止する。成功時のピーク容量を実測したわけではない。大容量の有料runnerへ自動変更しない。

出典: [公式runner仕様](https://docs.github.com/en/actions/reference/runners/github-hosted-runners)、[公式macos-26イメージ](https://github.com/actions/runner-images/blob/main/images/macos/macos-26-arm64-Readme.md)、[Actions料金・無料枠](https://docs.github.com/en/billing/concepts/product-billing/github-actions)。private repositoryの実行/保存は契約と残枠により課金対象になり得る。今回はpush・実行・公開・課金許可を確認できていないので**ローカル作成まで**。

## 次の最小操作

1. 利用する非公開GitHub repositoryと、Actions実行可否（無料枠/予算）を指定する。こちらから無断実行はしていない。
2. 許可後、成果物ハーネスをそのrepositoryへ配置。まずtarget=uiでBをbuildし、ログを確認。元の構成の比較にはbaseline、公開core単独にはcoreを使う。
3. A/B/coreを通したらallでCを生成。失敗時はログの最初のcompile/configure errorから直す。IPAが得られても未署名で、そのまま端末実行できない。
4. 署名・installが可能になってから、新規実験appへ自分のROM/必要system data/通常saveの**バックアップコピー**を配置。候補はDocuments/RoomExperiment-v1/3DS配下。CIA導入は未対応。旧stateをコピーしない。自動同期や本番セーブを実験に使わない。
5. オフライン起動・日本語キーボード・画面/タッチ/音声/JITを先に確認。その後PCの同じprotocol4 Roomへ同一LANでjoinし、最後にゲーム内通信を確認する。

ローカルMacでもci/prepare.pyで新しいcheckoutを用意できる。macos-preflight.shはGITHUB_ENV/PATH前提なのでCI向け。手動ではXcode/CMakeを同じ版に合わせ、changes/ManicEMU/Scripts/Room/build-app.shを使う。
