# 公開候補コアの互換性調査（段階A2）

2026-09-23。元コア対応ソースからの改修と、別の公開コアによる実験を分離する。以下の一致は呼び出し側と固定公開ソースの比較。非公開dirty差分や同梱バイナリ全体との意味的同一性は保証しない。

固定点: Manic fbaeab79c214d5920bb51afa6f2d786fb2b12a58、Libretro 00689c83f4d458e061d5fd7b52a181b8d240fe65、Daiuno/azahar 03da4a354cc10012418c28a935dd0c627e37a963、libretro-common 5e54bdaed73d885d5f7dae0ca489c0c3f85ff268。最後の固定submoduleも取得して実ヘッダーを比較した。

## 7 APIの対照表

| Manic要求API | 実際の呼び出し元 | 期待型・構造体 | 公開候補の処理 | ABI差 | 動作差 | 互換処理 | 未確認事項・必要なテスト |
|---|---|---|---|---|---|---|---|
| retro_azahar_extension_version | 固定Manic/Libretro検索で呼出なし。元binary exportのみ | 宣言なし。名前から戻り値型も推定しない | なし | 契約不明 | 数値の意味も利用側も未発見 | 未実装・未export。retro_manic_experiment_api_version=1は今回定義した別契約 | メンテナーのheader/契約取得。将来call追加時に再審査 |
| retro_azahar_set_keyboard_callback | PlayViewController起動完了→LibretroCore.registerAzaharKeyboard。stop時nil | void(callback(const config*))。C configは下表 | retro_set_keyboard_callback→AppleKeyboard登録 | 名称差。フィールド配置一致 | DefaultAppletsを置換。ロード完了後登録、次のretro_run以降で要求 | フィールド単位コピーの薄いadapter。ポインター再解釈はしない | iOS arm64、ゲーム中表示、停止直後の遅延callback、再ロード |
| retro_azahar_keyboard_input | ThreeDSKeyboardView完了→LibretroCore.inputAzaharKeyboard | void(const char*, int)。UTF-8。NULL→空文字 | retro_keyboard_input→SubmitInput→Finalize | 名称差。int32 buttonの意味一致 | 同一エミュレーションスレッドが前提。公開版profanity filterはTODO | 登録callbackとbutton0〜2を確認して実処理へ転送。mainに正規化、世代違い回答を排除 | Single/Dual/Triple/None、Cancel、日本語・UTF-16長・空白・固定長・ゲーム検証callback |
| retro_azahar_install_cia | PretendoNetworkingView→LibretroCore.installAzaharCIA。直接呼出はNimbus導入 | void(const char*)、成否なし、起動前に独立dylib_load | retro_install_cia→AM::InstallCIA(path,progress) | 引数は一致するが初期化契約が欠落 | 保存先未設定、既存Systemとの共存が不明。AM本体のInstallStatusが捨てられる | 未対応・未export。実験アプリはdylib_load前にalertで説明して中止。Bは既存経路 | 専用保存先＋成否API、電源OFF導入、暗号化/破損/中断、更新認識。単純別名は危険 |
| retro_azahar_load_amiibo | LibretroCore.loadAmiibo→ui_cocoatouch.m | C bool(const char*)、ObjC BOOLへ値変換 | retro_load_amiibo→NFC_U::LoadAmiibo | C bool1 byte、名称差 | PoweredOn/nfc:u確認。原core内部は未確認 | NULL拒否して実処理転送、mainへ正規化 | 起動前false、検索中/外、無効ファイル、停止競合、実機タグ読取 |
| retro_azahar_is_searching_amiibo | LibretroCore.isSearchingAmiibo→ui_cocoatouch.m | C bool(void) | retro_is_searching_amiibo→NFC_U::IsSearchingForAmiibos | C bool一致、名称差 | 電源OFFはfalse | 実処理転送。main外同期呼出はNOとログ | 検索開始/終了UI、ロード前後、再初期化 |
| retro_azahar_remove_amiibo | 呼出なし、元binary exportあり | 公開側void(void)。Manic期待型は宣言なしで確定不能 | retro_remove_amiibo→NFC_U::RemoveAmiibo | 公開型を採用、元binary型は未確認 | 電源/NFCサービス確認 | 未対応・未export。公開側void(void)は確認できるが、元契約を確定できず、使用中のManic呼出もないため保留 | タグ読取後の取り外し、電源OFF、複数回 |

## キーボード構造体、値と寿命

Windows x64 MSVCで実ヘッダーと共有adapter headerを比較。sizeof=56、alignof=8、int=4、bool=1、pointer=8。iOS arm64では同じ値をstatic_assertするworkflowを用意したが、まだ実行していない。

| フィールド | offsetof | 型 |
|---|---:|---|
| button_config / accept_mode | 0 / 4 | int |
| multiline_mode | 8 | bool |
| max_text_length / max_digits | 12 / 16 | int |
| hint_text / button_text | 24 / 32 | const char* / const char** |
| button_text_count | 40 | int |
| prevent_digit / prevent_at / prevent_percent / prevent_backslash | 44 / 45 / 46 / 47 | bool |
| prevent_profanity / enable_callback | 48 / 49 | bool |

paddingは9〜11、20〜23、50〜55。Frontend::KeyboardConfig内部のu16とC ABIのintを混同しない。ObjC NS_ENUMはNSUInteger（arm64で8 byte）だがC構造体にはintを使い、値を変換する。

ButtonConfigはSingle=0/Dual=1/Triple=2/None=3。AcceptedInputはAnything=0〜FixedLength=4。返すbuttonはSingle OK=0、Dual Cancel=0/OK=1、Triple Cancel=0/Forgot=1/OK=2、Noneは既存Manicどおり0。表示button_textはHLE由来の3要素（Cancel/Forgot/OK）。

callback configはスタック、文字列/配列はAppleKeyboard所有。次のExecute/破棄までで、adapterはcallbackの戻りまでしか借用を保証しない。ObjCはmainへdispatchする前にNSString/NSArrayへ同期コピーする。入力UTF-8は関数内でstd::stringにコピー。公開版のFinalize/data_readyは非atomicなので、UIとemulationの同時呼出は禁止。ManicのCocoaTouch pumpはCFRunLoopGetMainで、今回このexecutorへ操作を正規化。登録解除・停止後のqueued表示と古いゲームの回答は世代で排除する。実機の全呼出経路を検証済みとはしない。

## API名以外の機能差

| 機能 | 確認した経路 | 実験版・残る確認 |
|---|---|---|
| 描画・配置 | SpecialCoreOptionのcitra_layout_option/custom_layout_config→core_settings→EmuWindow_LibRetro::UpdateLayout。GL/Vulkan/software | 上下画面、縦横回転、解像度、swap、MoltenVKを実機比較。名称一致では描画互換を保証しない |
| 入力・タッチ | ThreeDS.swift AzaharEmulatorBridge→Libretro buttons/analog/touch→input_factory/emu_window | ボタン対応を確認。座標倍率・touch・controller実機試験が必要 |
| 音声 | libretro_sink/SubmitAudio→RetroArch driver | sample rate、途切れ、復帰、microphoneは未検証 |
| JIT/interpreter | Manicのcitra_use_cpu_jit→core_settings。Daiuno/dynarmic/oaknut gitlink | 実機JIT可否・JITなし性能は不明。新規署名/JITツールは作らない |
| 更新データ | AM/NAND/SDMCのinstalled title→loader | CIA API未対応。自分のバックアップを実験用領域へコピーして認識確認。v1.2等を未確認で同一と扱わない |
| 通常セーブ | Libretro save dir→ParseStorageOptions→FileUtil→NAND/SDMC | Documents/RoomExperiment-v1/3DS。専用pathでなければLoad拒否。自動移動なし。形式互換はタイトル単位で別途試験 |
| ステート | 元と候補でserialization/build IDが異なる | 実験版size=0、save/load=false。UIも除外。互換でないstateを読み込まない |
| チート | retro_cheat_*は空だがThreeDS.setupCheats→CitraCheatsManager→ファイル、候補System::Load→LoadCheatFile→RunCallbackの別経路あり | MANIC_ROOM_EXPERIMENTで実行loopを無効化しUIも除外。既存機能維持とは主張しない |
| 速度・巻き戻し | LibretroCore既存操作 | 実験アプリで常時制限。Room参加時も通常速度へ戻す |

元コア互換検査check-core-exports.shは厳格なまま残した。候補は合格しない。別契約check-experimental-core.shは「欠落が上記3 APIのみ」、他の旧export全て、4 adapter、4 Room API、別の実験API、iOS arm64を必須にする。輸出名検査は動作試験の代用ではない。
