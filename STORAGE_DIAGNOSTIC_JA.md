# C実験版：3DS保存先ガードの実機診断

基準commit `34afdc9e1442a57f9bad26b8ccf10cf1ee20c800` の実機ログでは、`do_load_game RETURN result=0` の前に `Core.System.Load ENTER` がない。現コードの制御経路では実験版UserDirガードが拒否した状態に一致する。CPU JIT判定、Vulkan context、ROM loaderが通過したことは確認済みだが、ゲームの起動は始まっていない。

実際のUserDir値は受領したログに含まれていない。今回の差分はその値がどこで変わるかを調べる。保存先ガード、`RoomExperiment-v1/3DS`、元framework、Room/CIA/JIT/adapterの契約は維持する。

新しい署名前IPA名は`ManicRoom-experimental-storage-diag-UNSIGNED.ipa`。以前の`ManicRoom-experimental-diag-UNSIGNED.ipa`とはSHAで区別する。

## 記録する値

`[ROOM-C-DIAG]`で以下を記録する。個人のcontainer UUIDを含む`Documents`より前の部分は`[path]`へ置き換える。

- `ParseStorageOptions GetSaveDir`：RetroArchがcoreへ返す保存先。
- `ParseStorageOptions GetSystemDir`：save dirが空の場合のみ既存処理で取得する。save dirが設定済みなら`<not queried; save dir provided>`と出す。
- `target_dir before append`／`after append`：`3DS/`を追加する前後。
- `CreateDir RETURN success=0`または`SetUserPath RETURN UserDir`：作成・設定後の実際のUserDir。
- `do_load_game storage guard UserDir`：ゲームを読み込む直前の実際のUserDir。

期待値は`[path]/Documents/RoomExperiment-v1`から`[path]/Documents/RoomExperiment-v1/3DS/`への変化。パスが`Documents`の外なら`[path outside Documents]`と記録するため、秘密のパス部分は出さない。末尾`/`の有無もログで判断する。

`GetSaveDir`が別の保存先を指している場合は、RetroArchの設定反映経路を調べる。`after append`は正しいのに`SetUserPath`が違えばAzaharのパス設定を調べる。末尾スラッシュのみの差なら、実際の比較を確認してから両辺を正規化する。元Manicの保存先へのフォールバックはしない。

## 次の実機操作

新IPAを既存の署名・JIT手順で導入し、アプリの新PIDについて**ゲームを押す前から**ログを取得する。同じゲームを一度だけ起動し、上の保存先ログと`do_load_game`／`Core.System.Load`のENTER・RETURNを確認する。個人パスやROM名を確認・除去したログを共有する。ROM・CIA・save・鍵は不要。Roomへの接続は今回行わない。

このCIではiPhone起動、保存先の実体、ゲーム起動を確認できない。保存先の修正は実機で観測した具体的な値に合わせて行う。
