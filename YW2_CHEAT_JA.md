# C実験版: YW2 Local Play Fix と Cheat

基準は `342ac600ee9708293e1da9026b261ae04ab2da68`。公開candidateは `Daiuno/azahar@03da4a354cc10012418c28a935dd0c627e37a963` に固定する。元framework、Room API、CIA、JIT、keyboard adapter、`RoomExperiment-v1/3DS/` の保存先は変更しない。

## 移植元と適用順

`utosa123/azahar` の `yw2-local-play-v3-beta1`、最終source commit `2b9131b881a7600089117fd5df55e1de57d18a6b` の NWM/Kernel 差分だけを `patches/YW2-local-play.patch` に抽出した。関連履歴は `8e16c580a8255b90d4ea97ea5fcba8d8d1bcec53`、`5a8ac5daf69810d1d1735263bb887ac819ecb3b1`、`d400c33b8a6b7b8301d62cb128cf208e0323bc6d`、`628be22fe9f5b99fdd7510e4a23e9bb2a9b03c67`、`1fd07089ed68954b39e72ef3d1d3e2accab3cb99`、`2146f62a21e3d8bcb00aadc9cd77a158c7f5f538`、`2b9131b881a7600089117fd5df55e1de57d18a6b`。

Core patch順: `azahar-experimental` → `YW2-local-play` → `CIA-core` → `Diag-core` → `Cheat-experimental`。Cheat patchは診断patchと `citra_libretro.cpp` 冒頭のcontextが重なるため、診断patchの後に適用する。Manic patch順は従来分の最後に `Cheat-Manic` を追加する。

YW2差分は、NWM_UDSの `0x0021` handler、`0x001D`/`0x001E` worker登録、対象title・同一process/thread・Dead・timeout=0・first pollだけのone-shot回避、process終了時の状態消去で構成する。日本版3件と最終source commitの海外版12件を列挙しており、海外版の実機動作はここでは未確認。

CheatはC実験版Azaharだけで、Manicの既存Cheat UIからlibretroの `retro_cheat_reset/set` へ送る。`+` を改行へ戻して既存 `GatewayCheat` に渡し、`CheatEngine` 内にfile-loaded一覧とは別のindex付きregistryを置く。resetはそのregistryだけを消す。RoomSessionがactive（join/joined/leave処理中）のcallbackはCheat実行を飛ばすが設定は保持するので、退出後の次のcallbackから再開する。ManicはRoom active中のCheat UI表示と既に開いた一覧での操作を制限し、coreもset/resetを拒否する。safeMode・hardcoreの既存制限は維持する。

`retro_serialize*`、Save/Load State、Quick Save/Load、State List、Fast/Slow Motion、RewindのC実験版制限は変更しない。静的検査とビルドは実機動作の代用ではない。Cheat ON/OFF、Joker、Room退出後の再開、通信対戦などは新IPAで改めて確認する。
