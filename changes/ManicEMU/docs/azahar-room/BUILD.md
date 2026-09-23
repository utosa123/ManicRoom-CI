> 段階Aの記録を保持しています。現行の追加実装・未対応範囲・実行手順は COMPATIBILITY_JA.md、BUILD_A2.md、LIFECYCLE_A2.md を優先してください。

# ビルド・引き継ぎ

**生成済みIPAはない。** このリポジトリのアプリ差分、Libretro submodule差分、公開候補Azahar差分は別々に管理する。対応ソース不足を解消するまで、候補frameworkを既存Manicへそのまま差し替えない。

## パッチの再現

成果物zipの`patches/`には3つのパッチ。各README記載の固定コミットへチェックアウトしてから適用する。既存変更がある環境では新しい作業ツリーを使うか、`git apply --check`で確認する。以下は新規開発フォルダーでの例。

```bash
GIT_LFS_SKIP_SMUDGE=1 git clone https://github.com/Manic-EMU/ManicEMU.git ManicEMU
git -C ManicEMU checkout fbaeab79c214d5920bb51afa6f2d786fb2b12a58
git -C ManicEMU submodule update --init --recursive
git clone https://github.com/Daiuno/azahar.git azahar
git -C azahar checkout 03da4a354cc10012418c28a935dd0c627e37a963
git -C azahar submodule update --init --recursive

# PATCH_DIRは解凍したpatchesディレクトリの絶対パス
git -C ManicEMU apply --check "$PATCH_DIR/ManicEMU.patch"
git -C ManicEMU/Dependencies/Libretro apply --check "$PATCH_DIR/Libretro.patch"
git -C azahar apply --check "$PATCH_DIR/azahar-experimental.patch"
git -C ManicEMU apply "$PATCH_DIR/ManicEMU.patch"
git -C ManicEMU/Dependencies/Libretro apply "$PATCH_DIR/Libretro.patch"
git -C azahar apply "$PATCH_DIR/azahar-experimental.patch"
```

Manicのgitlink自体は更新していない。依存の変更を親リポジトリだけで配布しないこと。対応するLibretro.patchも必ず含める。patchesにLFSバイナリやゲームデータは含めていない。

## Windowsで実施済み：通信部分だけのビルド

必要な候補依存は`externals/enet`、`externals/boost`、`externals/fmt`のみ。固定SHAはprovenance.json参照。

```powershell
git -C azahar submodule update --init --depth 1 externals/enet externals/boost externals/fmt
$cmakeExe = 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$ctestExe = 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe'
& $cmakeExe -S azahar/tests/room_bridge -B room-tests -G 'Visual Studio 17 2022'
& $cmakeExe --build room-tests --config Release -j 4
& $ctestExe --test-dir room-tests -C Release --output-on-failure
```

`room-tests/Release/room_bridge_tests.exe`はテスト実行ファイルでありエミュレータやIPAではない。loggerだけ同期テストsinkに差替え、Room/RoomMember/packet/Network/ENet/RoomSessionは実コードをコンパイルする。console identityはテスト専用値。core CFG/NWM全体のビルドは含まない。

## macOSで次に実行する実験コアビルド（未実行）

Mac、Xcode（上流の現在の案内に合わせ26系を候補とするが今回未検証）、iOS SDK、CMake 3.25以上、Git LFSと全固定submoduleが必要。`xcodebuild -version`、`xcrun --sdk iphoneos --show-sdk-version`の結果を必ず記録する。

```bash
bash ManicEMU/Scripts/Room/build-core-ios.sh "$PWD/azahar" "$PWD/artifacts/candidate-core"
```

上流候補のiOS workflowをもとにしたCMake/Xcodeコマンドをスクリプトに固定した。iOS arm64、deployment 15.0、Libretro、PCH OFF。ROM鍵blobを新たに含めないため`ENABLE_BUILTIN_KEYBLOB=OFF`。LTO後のexport存在を`nm`で確認する。コンパイル結果のdylibをframework化し、install nameを`@rpath/azahar.libretro.framework/azahar.libretro`に設定。署名しない。linked-libraries.txtで依存とrpathも点検する。

候補の出力予定：`artifacts/candidate-core/azahar.libretro.framework`、configure.log、build.log、環境情報、export、SHA256。**今回は未生成。** upstreamソースの既存ビルド問題が判明した場合はログを残して修正する。Windows実通信ビルドをiOSビルド成功と扱わない。

## 正しいコアの差し替え条件

1. 同梱バイナリに対応する`manic`ブランチ4523838e…とdirty差分を入手する。
2. そのソースへRoom API/lifecycle変更を移植。独自keyboard、amiibo、CIA、layout、JIT等を保持。今回の候補パッチは自動適用できる保証がない。
3. 全固定依存とframework生成手順を記録し、Macでビルドする。
4. originalとcandidateの公開API名を検査する。

```bash
bash ManicEMU/Scripts/Room/check-core-exports.sh \
  ManicEMU/Cores/azahar.libretro.framework/azahar.libretro \
  artifacts/candidate-core/azahar.libretro.framework/azahar.libretro
```

現在の公開候補は既存の7つの`retro_azahar_*` APIが不足するため、この条件を満たさない見込み（export list静的比較で確認）。名前が一致してもABIの意味まで同じとは限らない。frameworkを元の保存先へ置く作業は、これらの確認とバックアップ後に行う。既存署名をコピーせず、自分のビルドを使用する。

## Manicと署名前IPA（未実行）

全依存、全LFS、SPM packageを用意する。既存Xcode build phaseのVerifyLFS --fixも依存取得を試みる。`Scripts/VerifyLFS.sh`は事前に実行する。Secretsをログに出さず、Cipher.swiftのサービス設定・必要機能を確認する。不要な外部サービスを使わないビルドの設定も別途Macで検証する。

```bash
git -C ManicEMU lfs pull
bash ManicEMU/Scripts/VerifyLFS.sh
# YOUR_BUNDLE_IDを自分用の別IDに置換。旧コアではpreflightが停止する。
bash ManicEMU/Scripts/Room/build-unsigned-ipa.sh \
  "$PWD/artifacts/ManicRoom" YOUR_BUNDLE_ID
```

実際のschemeは`ManicEmuSideload`、configurationは`SideloadRelease`。スクリプトは`CODE_SIGNING_ALLOWED=NO`でarchiveし、`Payload/ManicEmuSideload.app`をzip化する。Room APIを持たない旧コアはpreflightで拒否する。再署名ツール、証明書、Apple Accountをスクリプトに組み込んでいない。

予定出力：`artifacts/ManicRoom/ManicRoom-unsigned.ipa`、`ManicRoom.xcarchive`、archive.log、packages.log、build-environment.txt、SHA256SUMS。**今回はいずれも未生成。** プロビジョニング・対応entitlement・署名を追加した署名後IPAは別生成物で、ハッシュも変わる。署名前IPAの生成はインストール成功・JIT成功・通信成功の証明ではない。

## 変更前の実行結果

Windowsで候補コア全体を`ENABLE_LIBRETRO=ON`、`ENABLE_TESTS=OFF`、`ENABLE_BUILTIN_KEYBLOB=OFF`、VS2022 generatorで構成したが、多数の未取得submodule（nihstro/dynarmic/cryptopp/MoltenVK関連等）のためconfigure失敗。これは変更前からの環境不足で、今回のRoom変更によるビルドエラーではない。Manicの変更前ビルドはXcodeなしのため未実行。

通信だけのビルドではPCHなしで`common/logging/backend.h`の不足する`<string>` includeが判明し修正した。テストの最終結果はTESTS.mdと同梱ログを参照。
