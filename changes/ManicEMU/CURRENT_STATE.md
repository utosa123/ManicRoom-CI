# C実験への移行・B先行ビルドの準備（2026-09-23）

ユーザー判断によりBの通信対応化調査は終了。次はB target=uiを1回buildし、成功後にCを7工程で順次buildする。
push/Actions実行の許可と利用先repositoryは未指定。BとCのiOS buildはともに未実行で、成功とは報告しない。
新成果物ManicRoom-C-execution-readyに、A2由来ui workflowと、同一commitのB成功run/artifactを要求するc-after-ui workflowを準備。
Cはcore build→framework→Mach-O/依存→独自adapter→Room API→Manic→unsigned IPAの順。各工程の結果を別々に記録する。
元frameworkは移動せず保護し、Cだけ別staging copy・Bundle ID・RoomExperiment-v1保存先を使用。
extension_version/install_cia/remove_amiiboは未対応のまま。追加実装や成功stubなし。
実行手順と現在状態は新成果物READY_JA.mdを優先。ローカルのworkflow/検査script試験のみ実行済み。

---

# 元コアB方式の追加調査（2026-09-23）

結論: **今回の調査範囲では利用可能な入口を確認できない**。
BはUI/bridge＋同梱元framework、Cは別の公開実験core。ゲーム試験の段階名とは別。
元binaryのSHA256は183159290d777d42a68c17f5f4d90d8b88f7aa0281e4788bad4e5954a6df940cのまま。
実export trieは32個のretro_*だけ。NWMとRoomMemberの内部実装・g_room_member参照は存在するが、外部のInit/Join/Leave/取得契約を確認できない。
36 core options、environment callback、7独自APIにも利用可能なRoom制御経路を確認できない。
推測ABIによる呼出し・固定アドレス・別singletonは実装せず、B+は作成していない。
新成果物ManicRoom-B-core-investigationに調査書、再解析script、全symbol/export/import/逆アセンブルlog、A2からのUI表示差分を追加。
Windows静的解析と整合性検査を実行。Mac/iOS compile、runtime dlsym、Room参加、ゲーム通信は未実行。
A/A2成果物・Libretro bridge・公開候補Cの差分は維持。Bの既存build経路とC検査も維持。
次はBをUIビルド確認に限定し、通信実装はCを進めるのが現実的。B再開には同梱dirty buildの対応source/公開入口契約が必要。
詳細: docs/azahar-room/B_CORE_INVESTIGATION_JA.md（本調査を最優先）。push・Actions実行・公開・課金なし。

---

# 段階A2 現在状態（2026-09-23）

元frameworkと段階A成果物は保護。固定公開sourceは別の実験用core。
4 adapter実装、extension_version/CIA/remove_amiiboの3 APIは未対応。
Windows CTest 2/2 PASS。iOS build/端末/ゲーム未検証。
手動CI（Windows、A/B/C、candidate、API検査、unsigned IPA）をローカル作成済み、未push/未実行。
現在の手順はdocs/azahar-room/BUILD_A2.md、仕様はCOMPATIBILITY_JA.mdとLIFECYCLE_A2.md。
成果物はManicRoom-stageA2。次は非公開CI repositoryとActions実行可否の指定。

---
以下は段階Aの引継ぎ記録（保存）。現在の制限・手順は上記A2文書を優先。

# CURRENT STATE — Manic Azahar Room

更新：2026-09-23。段階Aの実験実装とWindows実通信テストを作成。**iOSアプリ/コアのフルビルド・IPA・実機通信は未完了。**

## 最大の阻害要因

Manic同梱Azaharの生成情報は`4523838e25be9c86696b7d684a136e09ed52de52` / branch `manic` / `4523838e2-dirty` / build `2026-09-08T03:09:53Z`。確認した公開reposから取得できない。対応コミットだけでなくdirty差分も必要。公開Daiuno master 03da4a3…は独自APIも異なるため同じコアではない。同梱frameworkは置換していない。

## できているもの

* Manicの専用Azahar Room項目と画面、old coreの未対応表示。
* 既存active Libretro handleに対する動的C ABI呼出し。別Azahar libraryは追加していない。
* 公開候補ソース上のRoomSession、Network lifecycle、async join/cancel/leave、state/error/participants、停止時cleanup。
* 実Room/RoomMember/ENetを使ったWindowsテストと制御ENet fixture。Room参加/転送/失敗/キャンセル/再接続を検証。
* Mac用コアbuild、export確認、署名前IPAスクリプト（いずれもmacOS未実行）。
* docs/azahar-roomの調査・build・test・install手順。成果物zipには親/submodule/候補coreの3パッチとソース、ログ、provenance。

## 実行していないもの

Manic/modified Azaharフルbuild、Swift/Objective-C型検査、simulator、端末起動・回帰、PC→Manic LAN、ゲームUDS、妖怪交換/対戦/バスターズ、署名/インストール。段階CのiOSホスト機能未実装。参考forkのYW2 workaroundも未適用。

## 次に行う順序

1. 4523838e… + dirty差分の対応ソースと固定依存/ビルド情報を入手。
2. Mac/Xcode環境を用意。対応ソースへRoom差分を移植し、既存Manic APIと描画/入力/JITの維持を確認。
3. フルコア→framework→Manic→署名前IPAの順に実build。Swift/Objective-Cの未検出コンパイル問題、レイアウトもここで検証。
4. PC非公開Roomと実機で段階Aの全ケースを確認。
5. 段階Bで相手発見/通信開始/ゲーム操作を区別して試験。必要な場合だけYW2 workaroundを別最小差分として検討。
6. A/Bが通った後、同じNetwork::Roomのprivate Create→自己Joinを使って段階Cを追加。前面ホスト運用。

リモート変更・公開・有料契約はなし。秘密情報の入力は現段階では不要。成果物を通信完成版として配布しない。
