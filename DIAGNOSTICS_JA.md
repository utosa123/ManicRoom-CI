# 一時的なC起動診断ビルド

基準：CIA対応C成功 commit 427101c237f9410ad83dba8a677e782304e49937 / run 35992822533。
申告された症状：iPhone 12 / iOS 18.7.8、真打が黒画面。外部debugserver attach成功・ManicのJIT利用可能・safeMode OFF・game JIT ON。これらは利用者の観測であり、CIでは実機/JIT/ゲームを実行しない。

新IPA名：ManicRoom-experimental-diag-UNSIGNED.ipa。親子Bundle ID、RoomExperiment-v1/3DS、Room/CIA/既存adapter、JIT判定・CPU設定・GPU設定・ゲーム処理は維持。署名・実機導入・Room接続はCIで行わない。同じC Bundle IDなので、利用者が署名導入すると既存C版の置換になる。元Manic用IDに変えず、比較用の旧IPAとC内データを保管する。

## 起動経路

- Azahar選択：PlayViewController → Libretro bridge → dynamic core/symbol解決 → retro_init → retro_load_game → context_reset/do_load_game → Core.System.Load → retro_run/Core.System.RunLoop。
- Citra選択：PlayViewControllerのcitraCore/MTKView → ThreeDS.start → allocateVulkanLibrary/allocateMetalLayer → 非同期workerのinsertCartridgeAndBoot。
- C実験版Azaharは前者。後者のログが出ないこと自体は異常ではない。Azaharの実表示はlibretro CocoaView/contextであり、MTKView生成成功を架空に記録しない。

## ログ

すべての追加診断は [ROOM-C-DIAG] 付き。本体/bridgeはNSLog、iOS candidate coreはOS_LOG_TYPE_DEFAULT/public文字列で出力する。DEBUGビルド限定のLog.debugやstderrには依存しない。C＋Azaharだけbridge診断を有効にし、他coreのlog_verbosity/libretro_log_levelは既存値を維持する。3DS起動時は既存monitorも有効にする。

起動要求にはsafeMode、jitAvailable、gameJit、selectedCore、ROM basenameを記録。設定要求値、保存設定との解決後値、coreのCanUseJIT結果とcitra_use_cpu_jit最終値を別々に記録する。coreの最終enabledだけでもJITコード実行の証明ではない。JITを強制許可したりCPU backendを変更する処理は追加しない。

libretroから届いた全levelを既存callback内でNSLogへ転送する。既存4096 byte整形バッファの上限は維持、上流が出さないログは生成できない。転送するログの絶対pathは[path]に置換し、各行にprefixを付ける。新規起動ログはROM basenameのみ。OSによるログ取りこぼしがあり得るため、RETURN欠落だけで停止箇所を断定せず、近傍ログと再現時刻も確認する。

ENTER/RETURNを対にし、bool/statusを返す起動処理は結果も記録。C++例外の巻戻し時はRETURNではなくUNWINDを記録し、例外を握り潰さない。voidのRETURNは正常帰還であってゲーム起動成功ではない。ThreeDS.start RETURN/dispatch RETURNはworker完了ではない。

retro_runは最初の5回と以後300回ごと。その回の内部RunLoopは最初の3反復と以後10000反復ごとに記録する。ログ負荷を抑えるための間引きで、未記録のフレームもある。毎frameの成功ログによる性能評価はしない。

## 利用者による採取（今回未実行）

1. 新IPAとcoreのSHAを報告書と照合し、所有する署名環境で作業コピーを署名。JIT手順は現在成功している手順を利用し、変更しない。
2. 診断版を起動して、新しいPIDを確認。旧PIDを再利用せず、3DSを押す前から既存syslog採取を開始する。
3. 初回はprefixで採取元を絞りすぎず、プロセス/PIDの元ログを取得後に [ROOM-C-DIAG] で抽出する。NSLogとunified logの両方を扱える採取方法であることも確認する。
4. 同一ROM/region/update版、safeMode・gameJit・JIT付与条件で一度起動。CIA導入や設定変更を同時に行わない。RoomへJoinしない。
5. 起動を押した時刻と、最後のENTER/RETURN、初回retro_run、JIT最終値を確認する。名前/path等を共有前に確認し、ROM/CIA/save/鍵/pairing/Apple Account情報は送らない。

## 読み分け

| 最後の観測 | 次に調べる範囲（断定ではない） |
|---|---|
| 3DS game launch requestedがない | IPA/PID/採取開始時刻/選択経路、ログ採取方式 |
| dynamic core load ENTERのみ | dylib loader、署名、依存関係 |
| symbol resolve途中 | 最後のsymbolとcore/bridge整合性 |
| retro_init ENTERのみ | core初期化 |
| UpdateSettings/Loader.GetLoader/LoadKernelMemoryMode ENTERのみ | option適用・ROM loader |
| retro_load_game成功、context_resetなし | frontend/GPU context交渉。HWでは成功返却がboot完了を意味しない |
| Core.System.Load ENTERのみ | エミュレーション初期化。直前のcore転送ログを併用 |
| LoadDefaultDiskResources ENTERのみ | shader/resource初期化 |
| retro_run game_loaded=0 | 遅延boot失敗、context_reset/do_load_game結果 |
| Core.System.RunLoop ENTERのみ | 初回CPU/サービス/GPU実行。JIT関連かは未確定 |
| RunLoop RETURN反復、retro_run RETURNなし | frame提出待ちの可能性 |
| retro_run RETURN継続・黒画面 | frame内容/表示経路/ゲーム状態。JIT成功とは断定しない |

Room通信、CIA実導入、実機動作は今回のbuild/静的検査とは別。元frameworkと旧IPAは保持する。診断が終われば追加Diag patch3本と診断専用CI変更を戻すだけで基準ソースへ戻せる。
