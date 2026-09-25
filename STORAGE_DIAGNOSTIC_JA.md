# C実験版：3DS保存先ガードの実機診断

## diag2：frontend分岐が通らなかった原因

`ca8a33388e72e854e7fd2332c5add11ae1b0fa6b` の実機テストでも、`GET_SAVE_DIRECTORY`は`<C app Documents>/`を返した。core側には新しい`ParseStorageOptions`ログが出ており、`UserDir`は`<C app Documents>/3DS/`のため従来のガードで停止した。

Manicの`Game.libretroCorePath`は`Bundle.main.path(forResource: "azahar.libretro", ofType: "framework", inDirectory: "Frameworks")`を返し、**frameworkディレクトリ**をRetroArchへ渡す。RetroArchの`task_push_load_content_with_new_core_from_menu`はその値を`RARCH_PATH_CORE`へ設定する。前回のfrontend分岐は末尾を**binary名**`azahar.libretro`と比較したため、ここで誤って除外された。

diag2ではC実験版に限り、`RARCH_PATH_CORE`がアプリ同梱のAzahar frameworkディレクトリ、またはその中のAzahar binaryと**正確に一致**する場合だけ分岐を通す。custom save rootも同一アプリの`Documents/RoomExperiment-v1`との完全一致を要求する。任意のパスや他coreは許可しない。分岐を拒否した場合は個人パスを出さずに`corePath=missing/other`、`customSaveDir=missing/mismatch`を`[ROOM-C-DIAG]`へ記録する。保存先ガードは変更しない。

新しいIPA名は`ManicRoom-experimental-storage-diag2-UNSIGNED.ipa`。前回の`storage-diag`版と取り違えないこと。実機ではまず`frontend SAVE_DIRECTORY isolation accepted corePath=framework customSaveDir=expected`と`frontend SAVE_DIRECTORY=C Documents/RoomExperiment-v1 (isolated)`の有無を確認する。`accepted`が出ない場合は拒否理由を確認し、元保存先へのフォールバックで起動したものとして扱わない。

基準commit `34afdc9e1442a57f9bad26b8ccf10cf1ee20c800` の実機ログでは、`do_load_game RETURN result=0` の前に `Core.System.Load ENTER` がない。現コードの制御経路では実験版UserDirガードが拒否した状態に一致する。CPU JIT判定、Vulkan context、ROM loaderが通過したことは確認済みだが、ゲームの起動は始まっていない。

追加の実機ログで`SAVE_DIRECTORY`が`<C app Documents>/`を返していたことが分かった。RetroArchは`RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY`で現在の`runloop_st->savefile_dir`を返し、起動時に指定された`RoomExperiment-v1`がそこで使われていなかった。この値からcoreが`3DS/`を追加しても、`<C app Documents>/3DS/`となり隔離ガードに拒否される。

前回版はC実験版かつ読み込むcoreのbasenameが`azahar.libretro`で、起動時に保持したcustom save dirがそのアプリの`Documents/RoomExperiment-v1`と一致する場合に限り、`GET_SAVE_DIRECTORY`へそのrootを返す設計だった。しかし上記のframeworkディレクトリ経路を見落としていた。保存先ガード、`RoomExperiment-v1/3DS`、元framework、Room/CIA/JIT/adapterの契約は維持する。

前回の署名前IPA名は`ManicRoom-experimental-storage-diag-UNSIGNED.ipa`。以前の`ManicRoom-experimental-diag-UNSIGNED.ipa`とはSHAで区別する。

## 記録する値

`[ROOM-C-DIAG]`で以下を記録する。個人のcontainer UUIDを含む`Documents`より前の部分は`[path]`へ置き換える。

- `ParseStorageOptions GetSaveDir`：RetroArchがcoreへ返す保存先。
- `ParseStorageOptions GetSystemDir`：save dirが空の場合のみ既存処理で取得する。save dirが設定済みなら`<not queried; save dir provided>`と出す。
- `target_dir before append`／`after append`：`3DS/`を追加する前後。
- `CreateDir RETURN success=0`または`SetUserPath RETURN UserDir`：作成・設定後の実際のUserDir。
- `do_load_game storage guard UserDir`：ゲームを読み込む直前の実際のUserDir。

期待値は`[path]/Documents/RoomExperiment-v1`から`[path]/Documents/RoomExperiment-v1/3DS/`への変化。パスが`Documents`の外なら`[path outside Documents]`と記録するため、秘密のパス部分は出さない。末尾`/`の有無もログで判断する。

`frontend SAVE_DIRECTORY=C Documents/RoomExperiment-v1 (isolated)`と`GetSaveDir`の値で、frontendからcoreへの受け渡しを確認する。`after append`は正しいのに`SetUserPath`が違えばAzaharのパス設定を調べる。末尾スラッシュのみの差なら、実際の比較を確認してから両辺を正規化する。元Manicの保存先へのフォールバックはしない。

## 次の実機操作

新IPAを既存の署名・JIT手順で導入し、アプリの新PIDについて**ゲームを押す前から**ログを取得する。同じゲームを一度だけ起動し、上の保存先ログと`do_load_game`／`Core.System.Load`のENTER・RETURNを確認する。個人パスやROM名を確認・除去したログを共有する。ROM・CIA・save・鍵は不要。Roomへの接続は今回行わない。

このCIではiPhone起動、保存先の実体、ゲーム起動を確認できない。新しいIPAが生成されても修正の実機効果は別途確認する。
