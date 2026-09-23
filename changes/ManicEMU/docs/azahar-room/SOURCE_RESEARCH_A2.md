# 同梱ソースの追加調査

生成情報4523838e25be9c86696b7d684a136e09ed52de52 / manic / 4523838e2-dirty。stageAのMach-O調査とcommit取得結果を再利用。同じhash検索を反復していない。

2026-09-23の公開GitHub REST API追加調査。生の応答はlogs/additional-source-research.json、additional-trees.json。

| 場所 | 範囲・結果 |
|---|---|
| Daiuno/azahar branches | 5件を再確認。manicなし |
| 2120/2121系tree | 固定00e3bbb…/1277e84…を追加取得。citra_libretro/manic専用pathなし。対応sourceの手掛かりなし |
| Daiuno/azahar tags | 100件枠、実数16。2120-rc1〜2122.1。Manic向けtagなし。全tag全履歴の網羅探索ではない |
| Daiuno/azahar PR/release | all PR100件枠、release20件枠、双方0件。添付sourceなし |
| Manic releases | 20件枠、13件。assetはIPA。対応core source添付は識別できず。GitHub自動Source archiveはManic treeで、Azaharの対応sourceとは別 |
| Manic PR | all100件枠、9件。#86 Artic Home UI、RomM、Dolphin、cheat separator等。対応source公開PRを一覧に発見できず。未公開/削除refは見えない |
| Daiuno/libretro-common | azahar branch=固定gitlink5e54bdae…。include/libretro.hの旧6関数とkeyboard structを確認。extension_version契約なし |
| 候補build・依存 | libretro.ymlのiOS/macOS、CMake、.gitmodulesを調査。Daiuno dynarmic/oaknutは固定gitlink。これらは4523838e core本体の代替ではない |

結論は「この表とstageAで確認した公開場所では、同梱dirtyビルドに対応するsourceを見つけられなかった」。どこにも公開されていないとは断定しない。commitだけ入手できてもdirty変更が別途必要。

問い合わせ草案（未投稿）:

> ManicEMUのAzahar frameworkへprivate Room joinを加える実験をしています。元frameworkを保持し、Daiuno/azahar 03da4a3は別コアとして扱っています。同梱arm64 binaryからg_scm_rev=4523838e25be9c86696b7d684a136e09ed52de52、branch=manic、description=4523838e2-dirty、build_date=2026-09-08T03:09:53Zを確認しました。対応source（dirty差分含む）、submodule SHA、iOS build flags、libretro_azahar.hの場所を教えていただけますか。特にextension_versionの型/値の意味、CIA導入前の保存先初期化、keyboard/amiibo APIのthread契約を確認したいです。

参照: [候補source](https://github.com/Daiuno/azahar/tree/03da4a354cc10012418c28a935dd0c627e37a963)、[Manic releases](https://github.com/Manic-EMU/ManicEMU/releases)、[Manic PR](https://github.com/Manic-EMU/ManicEMU/pulls?q=is%3Apr)、[固定header依存](https://github.com/Daiuno/libretro-common/tree/5e54bdaed73d885d5f7dae0ca489c0c3f85ff268)。
