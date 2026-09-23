> 段階Aの記録を保持しています。現行の追加実装・未対応範囲・実行手順は COMPATIBILITY_JA.md、BUILD_A2.md、LIFECYCLE_A2.md を優先してください。

# Azahar Room 段階A：調査と実験実装

調査日：2026-09-23。**iOSで動作確認済みの改造版ではない。段階Aは実装候補、B・Cは未完了。**

## 固定したソース・環境

|対象|確認結果|
|---|---|
|Manic|`Manic-EMU/ManicEMU` main `fbaeab79c214d5920bb51afa6f2d786fb2b12a58`|
|Libretro依存|`Daiuno/RetroArch` `00689c83f4d458e061d5fd7b52a181b8d240fe65`。Manicのgitlinkに固定|
|実験用Azaharソース|`Daiuno/azahar` master `03da4a354cc10012418c28a935dd0c627e37a963`|
|比較した現行上流|`azahar-emu/azahar` master `b8c29a64c306bac3866a5ed293ebee94ef6dcb00`|
|参考fork|`utosa123/azahar` `yw2-uds-worker-order-fix-v10` `d400c33b8a6b7b8301d62cb128cf208e0323bc6d`|
|実行環境|Windows、Visual Studio 2022 Community / MSVC 19.44.35228.0、Windows SDK 10.0.26100.0|
|使用したCMake|VS同梱 3.31.6-msvc6。PATH上のMSYS CMakeは4.0.2だが今回のビルドには不使用|
|Git|2.49.0、Git LFS 3.7.1。MSYS Gitのsubmoduleスクリプト不具合を避け、取得後はWindows Gitを使用|
|macOS / Xcode / Swift compiler|利用できる環境なし。接続済みMacも見つからず。Xcode/SDKの確認済み版はない|
|実機・ゲームデータ|今回のテストでは使用していない。ROM・BIOS・鍵・セーブは取得・追加していない|

新規の作業ディレクトリに既存リポジトリ・変更はなかった。祖先と取得したソース内にAGENTS.mdは見つからなかった。ManicのCONTRIBUTING.mdを確認。別タスクの既存記録は無関係なゲーム解析だったため変更しなかった。すべての変更はローカル。push、PR、Release、クラウドジョブ、署名は実行していない。

## 最重要：同梱コアを再現するソースが不足している【確認済み】

`Cores/azahar.libretro.framework/azahar.libretro` はLFS管理のMach-O arm64バイナリ（27,797,040 bytes）。SHA-256:

`183159290d777d42a68c17f5f4d90d8b88f7aa0281e4788bad4e5954a6df940c`

Mach-OのLC_SYMTABとセグメント配置から、`Common::g_scm_*`の実データを読み取った。

* `g_scm_rev`: **4523838e25be9c86696b7d684a136e09ed52de52**
* `g_scm_branch`: **manic**
* `g_scm_desc`: **4523838e2-dirty**
* `g_build_date`: **2026-09-08T03:09:53Z**
* `g_build_fullname`: **4523838**

`-dirty`は生成元に未コミット差分があったことを示す。コミットIDだけ入手しても同一ソースの再現には足りない。GitHub APIでこのコミットをDaiuno/azaharとazahar-emu/azaharに問い合わせた結果は422、Manic-EMU/azaharは404だった。確認した公開リポジトリからは対応ソースを取得できていない。

実frameworkは`retro_azahar_extension_version`、`retro_azahar_set_keyboard_callback`等を公開する。公開fork masterのexport listは`retro_set_keyboard_callback`等の古い名前であり、同一ABIではない。2122ブランチはLibretro frontendを含まず、`warmenhoven/dev/azahar_libretro`も古いAPI名だった。別ソースを同梱コアと同一扱いしていない。今回の作業では同梱frameworkを変更・差し替えしていない。

**必要な追加資料：4523838e… + dirty差分、固定submodule、実際のframework生成手順。** 未公開の独自変更を推測して置換することは、この成果物の範囲外。

## Manicの構成【確認済み】

* `ManicEmu/ManicEmu.xcodeproj/project.pbxproj`: Azahar frameworkはEmbed Frameworks。アプリビルド中にAzaharソースをコンパイルする構成ではない。Libretro frontend自身は`Dependencies/Libretro`のソースからビルドする。
* `Sources/Tools/Cores/ThreeDS.swift`: `ThreeDS.isAzaharCore`でCitra専用bridgeと`AzaharEmulatorBridge`を切替。後者は`LibretroCore.sharedInstance()`へ入力を転送。
* `Sources/Business/Play/VIewControllers/PlayViewController.swift`: コア選択、3DS設定、JIT設定、Libretro起動・停止を管理。
* `Dependencies/Libretro/ui/drivers/LibretroCore.m`: `startWithCustomSaveDir:` → RetroArch起動、`stop` → callback解除とRetroArch停止。既存のkeyboard連携も`runloop_state_get_ptr()->lib_handle`に対して`dylib_proc`を使用する。
* `Dependencies/Libretro/runloop.c`: 実際のcore libraryのロードとLibretro lifecycle。今回のbridgeもこのロード済みhandleだけを使う。アプリに別のnetwork静的ライブラリをリンクしない。
* `EmulationCore.swift/supportNetplay`: Azaharは既存のRetroArch netplay対象外。`LibretroNetplayView.swift`は入力同期型の別機能で、Roomに流用していない。
* `Resources/Info.plist`: `NSLocalNetworkUsageDescription`が既に存在。既存Bonjourは`_manicemu._tcp`と`_ra_netplay._tcp`。IPv4直接接続に新規Bonjour登録・multicast entitlement・ATS例外は追加していない。
* JIT判定・StikJIT連携・描画/入力設定は既存コードを保持。新しいJIT有効化機能はない。
* `.gitmodules`には8依存。今回取得したのはLibretroのみ。他のManic submoduleとSPM packageは未取得。LFSはAzahar frameworkの3ファイルだけ取得、他はpointerのまま。全コアの大量取得はしていない。
* Windowsの長いパスによるcheckout欠落は新規リポジトリ単位の`core.longpaths=true`で修復。LFSのlocal設定で取得済みバイナリをソース差分から除外。
* READMEにはXcode 16+と26+の両記載、CONTRIBUTINGには16+。projectのschemeはLastUpgradeVersion 2640、Swift language modeは5.0、iOS deployment targetは15.0。Swift 5.9+のREADME表現はcompiler要件とlanguage modeを区別する必要がある。今回Xcodeでは検証していない。

## Azaharの経路【公開候補ソースで確認済み】

`src/network/network.cpp`の`Network::Init()`がENetとグローバルRoom/RoomMemberを生成し、`Shutdown()`が終了する。Initは冪等ではないため重複呼出し不可。

`src/core/hle/service/nwm/nwm_uds.cpp`はコンストラクタで`Network::GetRoomMember()`へWi-Fi受信callbackを登録し、デストラクタで解除する。ゲームの送信はNWM → `RoomMember::SendWifiPacket` → ENet → Room → 相手のRoomMember → NWM。このため、**NWM構築より前**にNetworkを初期化する必要がある。

`src/CMakeLists.txt`はnetworkを追加、`src/core/CMakeLists.txt`はnetworkをリンクする。`ENABLE_ROOM=OFF`は専用サーバー実行ファイルの設定であり、組込みRoom/RoomMemberが無いことを意味しない。

候補Libretro frontendの`retro_init`にNetwork初期化はなかった。Qtは`citra_qt.cpp`でInit/Shutdownし、`multiplayer/direct_connect.cpp`は別スレッドでJoin、状態callbackで結果を受ける。`host_room.cpp`はRoomをCreateしてホスト自身がJoinする。候補の古いAndroidにはemu_windowでのInitだけを確認した。現行上流のAndroid `jni/multiplayer.cpp`にはNetworkInit/Shutdown、NetPlayJoinRoom、NetPlayCreateRoomがある。後者はRoom作成後に同じRoomMemberで自己参加する。固定時間待機による判定は今回移植していない。

候補と現行上流の`network/room.h`はともにprotocol **4**、default UDP port **24872**。この一致は通信互換性の必要な手掛かりだが、PC版との実接続やNWM/ゲーム互換性の保証ではない。同梱dirtyコア内部のプロトコルはこのソース比較だけでは確定できない。

## 今回追加した経路【実装済み・iOS未検証】

```text
Azahar専用ゲームメニュー → AzaharRoomView
  → LibretroCore（main thread、active lib_handleを毎回確認）
  → versioned C ABI（同じazahar framework内部）
  → RoomSession worker → 既存Network::GetRoomMember()
                           ↑ ゲームNWMも同じshared instance
```

* ABI 1は固定幅整数・caller-owned snapshot bufferのみ。文字列は同期的にコピーし、境界にstd::string/クラス/callbackを出さない。APIは例外を外へ流さない。export listとdefault visibilityを追加。
* query/versionはcoreを初期化しない。旧コアにはシンボルがなく「未対応」。`dlopen`で別インスタンスを作らない。
* `retro_init`でNetworkを既存インスタンスがない場合のみInit。自分が所有した場合だけShutdown。
* ゲームのLoad成功後、emulation側でCFG console ID hashとMACを採取。worker側からSystemへアクセスせず、PC frontend同様にCFG MACをJoinへ渡す。
* Join戻り値0は受付だけ。snapshotはJoining、実際のJoined/Moderator callbackでのみJoinedになる。
* 状態・error・参加者名をmutex付きsnapshotへコピー。参加者の参照getterはMemberLoop内のroom callbackからしか読まない。UIはmain runloopで250msごとにコピーを取得。アプリへの非同期callbackは存在しない。
* IPv4 literal・1..65535・nickname `[A-Za-z0-9._ -]{4,20}`・password UTF-8 128 byte以内を検証。hostname/IPv6は今回対象外。passwordはログ・設定へ保存しない。
* Joinのtransport待機を25ms単位でキャンセル可能にし、失敗直後・loop thread未生成時にもLeaveできるよう修正。退出はworkerでsocket/threadを解放。既存呼出し向けの通常Leaveは維持し、bridgeだけimmediateモードを使う。
* transport接続後も10秒以内にJoinSuccessが来なければerror 101。通常のtransport接続待機は既存の5秒。
* ゲーム停止ではworkerをdrain → System ShutdownでNWM解除 → Network Shutdownの順。ライブラリを閉じる前にcallback・threadを解放。
* `NWM_UDS::GetMacAddress`はJoiningを接続完了とみなさず、Joined/ModeratorまでCFG MACを使う小修正を追加。これは未割当MACの読取り防止で、下記YW2 workaroundとは別。
* 接続中に画面を閉じるとキャンセル。参加後に閉じると接続を保ちゲーム再開。バックグラウンド通知では退出要求を送る。バックグラウンドホスト継続は想定しない。
* 時間変更とステート操作をLibretro bridgeとcore側で制限。接続受付時に早送り・スロー・巻き戻しを解除。設定ファイルを上書きせず、退出後の速度/巻き戻しはユーザーが必要に応じて再設定。
* 状態ログは`AzaharRoom state=… error=… protocol=…`とNetworkの`Room bridge …`。秘密情報・endpoint・console IDを追加ログに出さない。

## 妖怪ウォッチ2の参考fork【調査のみ・未適用】

masterの上流差分はREADME変更だけ。通信修正は別ブランチにある。

|コミット|変更内容|候補・現行上流|
|---|---|---|
|`8e16c580a8255b90d4ea97ea5fcba8d8d1bcec53`|NWM SetProbeResponseParam (0x0021)を成功応答stubにする|両方ともhandlerがnullptrで同等実装なし|
|`5a8ac5daf69810d1d1735263bb887ac819ecb3b1`|真打title ID 0004000000155100のhost/client UDS workerを追跡。完了threadの特定PC/LRからのzero-timeout pollingに一度timeoutを返すworkaround|該当workaroundなし|
|`d400c33b8a6b7b8301d62cb128cf208e0323bc6d`|v1.0のPC/LR条件を追加（v1.2条件も保持）|該当workaroundなし|

変更対象はkernel.cpp/h、process.cpp、svc.cpp、nwm_uds.cpp/h。OS専用APIではないためiOSへ移植できる可能性はある【推測】が、ゲストコード位置・ゲーム版・JIT/実行順・セーブ互換に依存する。今回iOS適用/ビルド/ゲーム検証なし。同梱dirtyコアに同等修正があるかはソース不足で未確認。成功応答stubは実3DS機能の完全実装ではない。段階Bで止まる症状をログと再現条件で確認してから、別パッチとして検討する。

## 主要根拠

* [Manic固定コミット](https://github.com/Manic-EMU/ManicEMU/tree/fbaeab79c214d5920bb51afa6f2d786fb2b12a58)
* [公開候補Azahar](https://github.com/Daiuno/azahar/tree/03da4a354cc10012418c28a935dd0c627e37a963)
* [現行上流network](https://github.com/azahar-emu/azahar/blob/b8c29a64c306bac3866a5ed293ebee94ef6dcb00/src/network/room.h)
* [現行上流Android](https://github.com/azahar-emu/azahar/blob/b8c29a64c306bac3866a5ed293ebee94ef6dcb00/src/android/app/src/main/jni/multiplayer.cpp)
* [YW2修正ブランチ](https://github.com/utosa123/azahar/tree/d400c33b8a6b7b8301d62cb128cf208e0323bc6d)
