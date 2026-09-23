# 段階A2 作業報告

既存段階Aの差分・成果物と元frameworkを維持し、公開候補を別の実験用コアとして進める実装、独立ビルド経路、手動CIを追加しました。**iOSのビルド成功・IPA生成・端末動作はまだ確認していません。** ソース不一致を理由に全作業を止める状態から、A/B/Cを別々に実行してログで進められる状態へ進めています。

## 1. 同梱コア追加調査

同梱は4523838e25be9c86696b7d684a136e09ed52de52 / manic / 4523838e2-dirty。追加でDaiuno/azaharの16 tag、PR/release（各0）、前回未確認2120/2121 tree、Manicの13 release/9 PR、固定libretro-common headerを確認。確認した範囲で対応source/dirty差分を発見できませんでした。「どこにも非公開」とは断定しません。問い合わせ文はSOURCE_RESEARCH_A2.mdへ保存し、未投稿です。

## 2. 独自API互換性表

全7 APIの呼出元、型/構造体、公開処理、ABI/意味差、実装・未確認・試験はCOMPATIBILITY_JA.mdに記載。

| API末尾（retro_azahar_） | 結果 |
|---|---|
| set_keyboard_callback | 実ヘッダー比較＋フィールドコピーadapterを実装 |
| keyboard_input | button意味を照合し実処理へ転送。mainへ直列化 |
| load_amiibo | C bool/const char*を照合し実NFC処理へ転送 |
| is_searching_amiibo | C bool(void)を照合し実NFC状態照会へ転送 |
| extension_version | 呼出・宣言・数値意味を確認できず未export。適当な値で偽装しない |
| install_cia | pre-init保存先/成否契約未確認。未export、実験アプリでは説明alertで中止 |
| remove_amiibo | 公開側処理はあるがManic期待型/呼出なし。元ABIを断定せず未export |

Windows x64のkeyboard sizeof=56、alignof=8、14 fieldのoffsetof一致をコンパイル確認。iOS arm64検査はworkflowに追加して未実行。enum/BOOL/C boolを混同せず、文字列はcallbackから戻る前にコピーします。

## 3. 実装と制限

4 adapterの実処理、元とは独立の実験契約識別API、shared ABI header、main外操作の正規化/拒否、keyboard世代管理、RoomSession停止〜解放のロック統合、繰返しGameReady時の旧worker終了を追加しました。保存先文字列の所有と、旧NAND/SDMC pathが残る経路も修正しました。

実験用appは別ID/表示名とDocuments/RoomExperiment-v1/3DSを使い、専用rootでなければゲームLoadを拒否。元framework/通常saveの自動置換・移動なし。cheatファイル経路も調査し、実験coreでcheat実行を止めました。state読書き・早送り/巻き戻し/slowも制限。画面/入力/音声/JIT/更新/通常saveの完全互換は未確認です。ゲーム起動/keyboardは保持しており、未検証のまま完成扱いしません。CIA導入は未対応なので更新dataは自分のバックアップコピーで個別確認が必要です。

## 4. 実行したビルド・テスト

* Windows MSVC 19.44、SDK 10.0.26100.0、CMake 3.31.6-msvc6、Releaseで実Room/ENet試験とkeyboard ABI検査、adapter翻訳単位をbuild。
* CTest **2/2 PASS、15.98秒**。実Room参加、password不一致、参加者、WiFi packet、退出/再接続、host消失、cancel/timeout/version不一致に加え、snapshot別thread＋100回停止/再初期化を検査。
* actionlint 1.7.7でworkflow検査。古いlinterがmacos-26を知らないため公式資料で確認したlabelをlint設定に追加。shellcheck/pyflakesは実行せず、Bash -n/Python compileを別途実行。
* 3 repoのdiff --check、3 patchの基準sourceへのapply --check→apply→内容比較、ABIコピー/export差分、元binary SHA256を検査（結果はprovenance.json）。
* 元binary SHA256は183159290d777d42a68c17f5f4d90d8b88f7aa0281e4788bad4e5954a6df940cのまま。

ネットワーク部品の成功は、エミュレータ全体・iOS・ゲームUDSの成功を意味しません。fixtureはprotocolエラー/timeoutを検証する相手であり、coreの成功返却stubには置き換えていません。

## 5. 未実行

A/B/CのXcode build、公開coreのiOS全体build、Swift/ObjC typecheck、iOS arm64 ABI compile、実Mach-O export/link依存検査、unsigned IPA生成、署名/install、実端末keyboard/GPU/audio/JIT/通常save、PC実機Room参加、YW2ゲーム内通信、ThreadSanitizer/ASan結合試験。手元WindowsにXcodeがなく、リモート実行許可も未確認のため、実行済みとは報告しません。

## 6. GitHub Actions

手動workflow完成（未dispatch）。Windows通信、A元構成、B UI＋旧core、candidate core arm64、独自/Room API検査、C変更app、unsigned IPAを分離しました。固定commit/gitlink、SPM lock、tool/image/SDK記録、patch事前チェック、容量preflight、失敗ログuploadがあります。BはUI・Room未対応表示。Cは専用契約の全必須exportを検査し、旧コアでの迂回は不可です。

公式macos-26 runnerにXcode26.6/iPhoneOS26.5 SDKがあることを確認。ただしhosted imageは変わるため実行時にpath/versionを検査。容量はLFS＋依存＋buildを事前確認し、有料runnerへ自動変更しません。push、workflow実行、公開、課金は行っていません。

## 7. 成果物

ManicRoom-stageA2.zipと展開済ManicRoom-stageA2/。REPORT_JA.md、CURRENT_STATE.md、追加調査/互換表/build/lifecycle文書、3 patch、全変更source、.github/workflows/room-experiment.yml、ci scripts、logs、provenance.json、SHA256SUMS。従来のManicRoom-stageA.zip/ディレクトリはそのまま残しました。新しいframework/IPAは含みません。

## 8. 次の最小操作

使用する非公開GitHub repositoryと、Actionsの実行可否（無料枠/予算）を指定してください。許可後はこの成果物を専用CIハーネスとして配置し、最初にtarget=uiでBのcompileを確認できます。baseline/coreを個別に進め、A/B/coreが通った後allでCを生成します。Mac購入や署名secretの準備を先に要求する構成にはしていません。
