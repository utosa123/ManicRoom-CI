# Bを先に1回確認してからCを実行する準備

2026-09-23。**準備・ローカル検査は完了。B/CのiOSビルドはどちらも未実行。**
利用先GitHub repository、push/dispatch、Actions利用枠の許可は会話内にないため、リモート操作は行っていない。B通信対応化の調査は終了。元coreへの内部呼出しは追加しない。

## 工程別の現在状態

| 工程 | 実行 | 結果 |
|---|---|---|
| workflow/解析検査の準備とローカル検査 | 実行済み | 成功（iOS buildではない） |
| B: 既存A2由来workflow target=ui | 未実行 | 未判定 |
| C1: 公開候補＋A2 Room差分のiOS arm64 build | 未実行 | B成功待ち |
| C2: framework生成 | 未実行 | C1待ち |
| C3: Mach-O export・依存関係 | 未実行 | 候補binary未生成 |
| C4: 独自API adapter・iOS keyboard ABI | 未実行 | 候補binary未生成 |
| C5: Room API export・既存C preflight | 未実行 | 候補binary未生成 |
| C6: C構成Manic build | 未実行 | C1〜5待ち |
| C7: unsigned IPA生成 | 未実行 | C6待ち |

元binaryを使った解析parserの比較検査や合成export集合の負例試験を、候補binary検査成功として数えていない。今回のbuild失敗は0件ではなく、**実buildを開始していないため成否未判定**。iOS compile、端末Room接続、ゲーム通信も未検証。

## 実行順序と成功の確認

このフォルダ全体が専用CIハーネス。workflowだけをManic repositoryに置かない。既存A2成果物自体は上書きせず、新しいハーネスを作った。

1. 許可後、このハーネスを指定された非公開repositoryへ配置・commitする。署名secret不要。`room-experiment.yml`の**target=uiを1回**実行する。A2のB build処理を維持し、B調査で追加した未対応文言patchを適用する。
2. B archiveとpackageが成功すると`B-ui-success` artifactを保存する。ui mode、元core SHA、別Bundle ID、run ID、ハーネスcommitを記録する。元coreを含むarchive内binaryにもSHA一致を要求するため、Xcodeがstrip等を行った場合は黙って承認せず停止する。
3. 同じハーネスcommitで`c-after-ui.yml`を実行し、`b_run_id`へ成功したBのrun IDを渡す。GitHub APIでrunの成功・repository・workflow・commitを照合し、B artifactも照合する。失敗中/未完了/別commit/別repository/証跡欠落ではCを開始しない。**Bを再ビルドせず、その成功記録を使用する。** artifact保持は7日。失効や修正でcommitが変わった場合はBを再確認する。
4. Cジョブは7工程を直列実行し、失敗した時点で後続工程を止める。`logs/C-stage-results.json`とActions Summaryへ各工程の実行済み/成功/失敗/未実行を出す。準備段階の失敗ではC工程は全て未実行となり、prepare/preflight logを見る。

許可後のCLI例（**未実行**）:

```bash
gh workflow run room-experiment.yml --repo OWNER/PRIVATE_REPO --ref BRANCH -f target=ui
# B runの完了と成功artifactを確認してから。自動でこの次の命令を実行しない。
gh workflow run c-after-ui.yml --repo OWNER/PRIVATE_REPO --ref BRANCH -f b_run_id=SUCCESSFUL_B_RUN_ID
```

旧A2のtarget=allではBとcandidate buildが並行していたため、新ハーネスの既存workflowはwindows/baseline/uiに限定した。Cは成功確認付きの別workflowへ移した。旧A2 ZIPとそのworkflowは保存済み。Windows transport試験は引き続き独立target=windowsで実行可能だが、今回は再実行していない。

## Cの保護と検査内容

* sourceはDaiuno/azahar `03da4a354cc10012418c28a935dd0c627e37a963`と既存A2 patch。Manic/LibretroもA2の固定commit。元4523838e dirty buildと同一とは扱わない。
* `src/ManicEMU/Cores/azahar.libretro.framework`を移動・変更・削除しない。C専用`stage-C/ManicEMU`を別コピーし、元frameworkをコピー対象から除外して候補frameworkを配置。前後で元framework全ファイルのhashを照合する。追加copy容量も事前確認し、不足で停止。有料runnerへ自動変更しない。
* Bは`local.manicroom.ui`、Cは`local.manicroom.experimental`。CはA2の`RoomExperiment-v1`保存先とチート/ステート/時間操作の制限を維持。利用者saveの移行なし。別Bundle IDによる将来の署名・entitlements調整は今回未検証。
* C1 dylib compilationとC2 framework packagingを分離。C3は実export trieを読み、thin ARM64/iOS device、install name、依存ライブラリを検査。未同梱の非system依存やre-exportは停止して個別判断する。
* C4は元coreの標準/独自exportのうち未対応3個以外の存在を必須にし、keyboardのsizeof/offsetofをiOS arm64 compilerで検査。adapter本体もC1でコンパイルされる。これは実機keyboard/amiibo動作の保証ではない。
* `extension_version` / `install_cia` / `remove_amiibo`は**引き続き未対応**。3 exportが追加されても検査を停止し、推測stubで通過させない。CIA操作を安全に止める既存Manic側処理を維持。Room接続試行だけに不要なこれらを実装するために推測しない。
* C5はRoomのversion/snapshot/join/leaveを必須とし、既存`check-experimental-core.sh`もそのまま実行する。Cの検査をB用に緩めていない。

macos-26の公式imageにはXcode 26.6とiPhoneOS 26.5 SDKが記載されていることを再確認した。A2の明示Xcode pathとCMake 3.31.6を維持し、実行時にXcode/SDK/compiler/image/submodule/SPMの情報を記録する。これはrunner確保やbuild成功の実測ではない。[公式runner image](https://github.com/actions/runner-images/blob/main/images/macos/macos-26-arm64-Readme.md)

B成功の検証は[GitHub workflow run API](https://docs.github.com/en/rest/actions/workflow-runs#get-a-workflow-run)と既存pinのdownload-artifactを使用。権限はcontents:read / actions:readのみ、checkoutの認証保持なし。リモートrun照合そのものはまだ実行していない。

## ローカルで実行済みの確認

* actionlint 1.7.7で2 workflowを検査（shellcheck/pyflakes連携なし）。shellは別途bash -n、Pythonは構文compile。
* 8 unittest成功。B失敗/未完了/別commit/別repo、adapter/Room欠落、未対応APIの追加、非system依存、re-exportを拒否する負例を含む。
* 元coreの実export trieを新検査parserで読み、B調査の独立二重解析済みlogと一致。合成candidate fixtureを実coreの代わりに生成・配布していない。
* A2の3 source patchを変更していないことと追加B UI patch適用を確認。元framework・過去成果物・Manic/Libretro/Cソース保全の結果は`logs/local-validation.json`。

旧A2試験logは`reference-logs/A2`へ分離。今回のlogは`logs/C-*`と`logs/local-validation.json`。全ての追加scriptはまだMacで実行したものではない。

次の最小操作は、**利用する非公開repositoryと、このハーネスのpush・Bのtarget=ui実行を許可する範囲の指定**。Cのdispatchも許可されるなら併せて指定する。許可がなければ、この実行直前の成果物で待機する。
