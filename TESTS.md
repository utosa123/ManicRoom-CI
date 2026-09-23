> 段階Aの記録を保持しています。現行の追加実装・未対応範囲・実行手順は COMPATIBILITY_JA.md、BUILD_A2.md、LIFECYCLE_A2.md を優先してください。

# テスト結果と実機チェックリスト

日付：2026-09-23。実施済みはWindows x64の通信テスト。iPhone、iPad、macOS、simulatorでは一切実施していない。

## 実施済み（成功）

`tests/room_bridge`をMSVC ReleaseでビルドしCTestを実行。テストは`assert`をReleaseで消す方式ではなく、失敗時に例外で終了コード1を返す。

* C ABI version、null snapshot、未初期化時の接続拒否。
* Init二重呼出しで同じRoomMemberを維持。
* IP不正、先頭ゼロ、IPv6（対象外）、port 0/65536、短いnickname、長過ぎるpasswordを拒否。
* 実Roomサーバー（loopbackのみ）へのJoin、wrong passwordエラー。
* 参加後にNetwork::GetRoomMemberが同一instance、CFG相当の指定MACを保持。
* 実RoomMemberをもう1つ接続し、2人の参加者情報と実Wi-Fi payload `{1,2,3,4}`の転送を確認。
* 接続中の重複Join拒否、退出、再接続、ホスト終了と切断理由。
* 応答のないportへのJoinキャンセルが1秒未満、キャンセル後に再試行。
* 応答のないportへの実5秒timeout、error 9。
* ENet制御fixtureからversion mismatch応答を送信しerror 7確認。**実PC版のバージョン違い試験ではない。**
* ENet transportだけ受け入れJoinSuccessを返さないfixtureで約10秒のhandshake timeout、error 101。
* 停止中Joinのdrain、10回のstop/ready反復、shutdown/reinitialize。

このWi-Fi payload試験はNWM/UDSのゲーム実行試験ではない。loggerはテスト専用同期sink、identityもテスト値。UI mockによる接続成功表示は使用していない。

## 失敗・解決した事項

* MSYS版gitのsubmodule script解決失敗 → Windows Gitで取得。
* Windowsの長いファイル名で新規checkoutの一部が失敗 → 当該repoのcore.longpaths設定と欠落パスだけの復元。
* 変更前コア全体のCMake configure → 未取得依存のため失敗、フルビルド未完了。
* 通信部分の初回コンパイル → PCH非使用時のbackend.h `<string>`不足、修正後成功。
* sandbox内のMSBuild → Windows SDK情報のアクセス拒否、通常環境で再実行し成功。

## 未実施：完了条件

|項目|状態|
|---|---|
|変更したAzaharフルコアのiOS/Windowsビルド|未実施（Windowsの変更前configureは失敗）|
|Manicアプリbuild、Swift/Objective-C型検査|未実施（Xcodeなし）|
|通常起動、描画、音声、入力、セーブ、JIT、通信しないプレイ|未実施|
|PC Azaharホスト → Manic参加、参加者リスト|未実施|
|実機上のIP/port不正、wrong password、timeout、version mismatch|未実施（通信単体の対応テストとは区別）|
|実機のキャンセル、退出、再接続、ホスト断、Wi-Fi断、背景/復帰|未実施|
|ゲーム内の相手発見|未実施|
|ゲーム内セッション開始|未実施|
|真打の妖怪交換|未実施|
|真打の対戦|未実施|
|真打のバスターズ|未実施|
|通信終了後の通常プレイ・ゲーム内セーブ|未実施|
|Manicホスト → PC／Manic参加、ホスト終了|未実施、段階C未実装|
|IPA生成、署名、インストール、署名更新|未実施|

## 正しいコアと実機ビルドが用意できた後の最小操作

1. 両端のゲーム内セーブをエクスポートして保管。同じゲーム版/更新版を利用者が用意する。
2. PC Azaharで非公開Roomを作成し、LAN IPv4、UDP port、passwordを確認。最初は同じWi-Fi。テストに伴うWindows firewall設定は利用者が確認する。
3. ManicでAzaharコアを選んでゲームを起動。通常の描画・入力・音声をまず確認。
4. ゲームメニューの「Azahar Room (3DS LAN)」を開く。未対応表示ならコアを確認してここで中止する。
5. IP/port/別々のnickname/passwordを入力し接続。ローカルネットワーク許可を求められたら許可する。「接続中」では成功扱いにしない。
6. **Room参加済み表示**と**PC参加者一覧**を別々に確認。画面を閉じてゲームを再開。
7. ゲーム内でローカル通信を選択し、**相手発見→セッション開始→交換/対戦/バスターズ成立**を別々に記録する。
8. 退出して再接続。その後ホスト終了、背景移行/復帰を試す。背景復帰後は手動再接続し、セッション継続を前提にしない。

console ID collisionはnickname変更で解決する問題ではない。データ一式の複製による本体ID/MACの重複を確認し、むやみにセーブや設定を消さない。

## 問題時に必要な情報

Manic commit、コアcommitとdirty差分識別、framework SHA256、Room API version/protocol、PC Azaharのversion/commit、iOS/device、JIT有無、ゲーム版・更新版、LAN構成、操作時刻、どの段階で止まったか。

Mac Console/Xcode device consoleの`AzaharRoom`、coreのNetwork/Service_NWMログ、PC側Room/NWMログを同じ時刻帯で揃える。state番号は0未準備/1未接続/2接続中/3参加済み/4退出中。errorはroom_api.hとUIのerrorText参照。参加が成功しゲームだけ失敗するなら、UDS command 0x001D/0x001E/0x0021とkernel waitの周辺を追う。

password、Apple Account、秘密鍵、pairing file、ROM、セーブをログ提出へ含めない。既存上流のverbose logには他の識別情報が入る可能性があるため、共有前に確認する。
