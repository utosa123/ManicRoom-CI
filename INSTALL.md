> 段階Aの記録を保持しています。現行の追加実装・未対応範囲・実行手順は COMPATIBILITY_JA.md、BUILD_A2.md、LIFECYCLE_A2.md を優先してください。

# 配布・導入方針（生成物未作成）

現在、配布可能なIPAは生成していない。この文書はビルドが成立した後の手順区分であり、特定の署名ツールやiOS版の動作保証ではない。

1. **IPA入手**：対応ソース、変更差分、ビルド情報、ハッシュを添付した署名前IPAを利用者へ渡す。ROM/BIOS/鍵/利用者セーブは新たに同梱しない。元の著作権・LICENSE/THIRD_PARTY_NOTICESを保持。
2. **署名**：利用者自身が選んだ外部ツールと本人の資格情報で署名する。Manicに署名ツールを組み込まない。開発者がApple Account、秘密鍵、pairing fileを収集・保管しない。
3. **インストール**：署名後に端末へ導入。署名前と署名後のIPAは異なる生成物として保管する。起動できたことを別に確認する。
4. **署名更新**：選択した方式の有効期限・更新条件に従う。今回ツール別の最新条件は調査・検証していないため、具体的な期限や完全PC不要対応を断定しない。
5. **pairing file**：採用したインストール/JIT方式が必要とする場合のみ、端末ごとに利用者が管理する。1人分のファイルを他人と共有する前提にしない。
6. **JIT**：インストールと別条件。Manicの既存JIT経路を使い、対象iOS/端末で確認する。未確認の新規ツールや証明書検証回避は追加しない。
7. **ゲーム内通信**：インストール・JITとは別テスト。TESTS.mdのRoom→参加者→発見→ゲームセッション→実操作を実施する。

SideInstaller / SideStore / ESignなどの最新対応表や個別導入案内は今回作成していない。必要になった時点で利用者が選んだツールの公式README/Releaseと実機条件を確認する。外部ツールのソースをManicへ取り込む必要はない。

## 別アプリとしての併存

`Resources/Config-Base.xcconfig`の既定`APP_BUNDLE_IDENTIFIER`は`com.aoshuang.manicemu`。build-unsigned-ipa.shは自分用の別IDを必須とする。単にBundle IDを変えるだけでは、hard-codeされたApp Group `group.aoshuang.manicemu`やiCloud `iCloud.com.aoshuang.manicemu`を利用できるとは限らない。署名方式に合わせ、entitlementと関連コード・サービス設定を確認する。CloudKit/App Groups等を必要なくする設定はMacで別途検証する。

通常の保存先は`Constants.swift/R.Path.Document`下の`Datas`、`3DS`等。別Bundle IDではsandboxが分かれ、公式版セーブが自動的に現れるとは想定しない。同一Bundle IDでも署名者/導入方式によって上書き可否が変わるため保証しない。

**移行前に公式版の共有/エクスポート機能（GameOption.shareSave → ShareManager）でゲーム内セーブをバックアップ**し、元アプリを保持して別IDへインポートして確認する。3DSの保存構造と本体IDに注意し、異なるコア版のステートだけを唯一のバックアップにしない。共有App Group/iCloudを未検証の改造版へ向けて同時同期しない。実際の移行は今回未検証。

Manic本体のAGPL-3.0、AzaharのGPL-2.0-or-later等、実際に配布する全コンポーネントのライセンスと対応ソースを保持する。今回の成果物はバイナリ再配布ではなくソース差分で、親リポジトリと変更したLibretro submoduleの両方を含む。
