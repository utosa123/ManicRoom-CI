// SPDX-License-Identifier: AGPL-3.0-or-later
import UIKit

/// Direct Azahar Room transport. Separate from RetroArch's input-sync netplay.
final class AzaharRoomView: BaseView {
    private let host = UITextField()
    private let port = UITextField()
    private let nickname = UITextField()
    private let password = UITextField()
    private let status = UILabel()
    private let members = UILabel()
    private let join = UIButton(type: .system)
    private let leave = UIButton(type: .system)
    private var timer: Timer?
    private var hideCompletion: (() -> Void)?
    private var didFinishHiding = false
    private var lastStatus: NSDictionary?

    required init?(parameters: Any...) {
        super.init(frame: .zero)
        let navigation = ASNavigationView(.defaultNavigation(
            title: "Azahar Room (3DS LAN)", titleIcon: .symbolImage(R.image.online_iconSymbols())))
        navigation.didTapClose = { [weak self] in self?.hide() }
        addSubview(navigation)
        navigation.snp.makeConstraints { make in
            make.top.equalToSuperview().offset(R.Size.SheetGrabberTopInset)
            make.leading.trailing.equalToSuperview()
            make.height.equalTo(R.Size.NavigationHeight)
        }
        let scroll = UIScrollView()
        scroll.keyboardDismissMode = .interactive
        addSubview(scroll)
        scroll.snp.makeConstraints { make in
            make.top.equalTo(navigation.snp.bottom)
            make.leading.trailing.equalToSuperview()
            make.bottom.equalTo(keyboardLayoutGuide.snp.top)
        }
        let stack = UIStackView()
        stack.axis = .vertical
        stack.spacing = 12
        scroll.addSubview(stack)
        stack.snp.makeConstraints { make in
            make.edges.equalTo(scroll.contentLayoutGuide).inset(16)
            make.width.equalTo(scroll.frameLayoutGuide).offset(-32)
        }
        let detail = UILabel()
        detail.numberOfLines = 0
        detail.font = .preferredFont(forTextStyle: .footnote)
        detail.text = "同じWi-FiのPC Azaharルームへ接続します。参加後はこの画面を閉じてゲーム内のローカル通信を選んでください。バックグラウンド移行時は退出します。"
        stack.addArrangedSubview(detail)
        for (label, field) in [("ホスト IPv4", host), ("ポート", port),
                               ("ニックネーム（半角4〜20文字）", nickname), ("パスワード（任意）", password)] {
            let title = UILabel()
            title.text = label
            title.font = .preferredFont(forTextStyle: .subheadline)
            stack.addArrangedSubview(title)
            field.borderStyle = .roundedRect
            field.autocorrectionType = .no
            field.autocapitalizationType = .none
            field.snp.makeConstraints { $0.height.greaterThanOrEqualTo(40) }
            stack.addArrangedSubview(field)
        }
        host.keyboardType = .decimalPad
        host.placeholder = "192.168.1.10"
        port.keyboardType = .numberPad
        nickname.keyboardType = .asciiCapable
        nickname.text = "ManicPlayer"
        password.isSecureTextEntry = true
        password.textContentType = .password
        join.setTitle("接続", for: .normal)
        join.addTarget(self, action: #selector(connect), for: .touchUpInside)
        leave.setTitle("キャンセル / 退出", for: .normal)
        leave.addTarget(self, action: #selector(disconnect), for: .touchUpInside)
        stack.addArrangedSubview(join)
        stack.addArrangedSubview(leave)
        status.numberOfLines = 0
        status.font = .preferredFont(forTextStyle: .body)
        members.numberOfLines = 0
        members.font = .preferredFont(forTextStyle: .footnote)
        stack.addArrangedSubview(status)
        stack.addArrangedSubview(members)
        let restrictions = UILabel()
        restrictions.numberOfLines = 0
        restrictions.font = .preferredFont(forTextStyle: .footnote)
        restrictions.text = "公開候補コアは実験用です。チート・ステート・速度変更は無効、保存先はRoomExperiment-v1です。CIA導入は未対応です。通信中はステート操作・早送り・巻き戻し・スロー・リセットを制限します。退出後、速度と巻き戻しは必要に応じて設定し直してください。ルーム参加だけではゲーム内通信の成功を意味しません。"
        stack.addArrangedSubview(restrictions)
        refresh()
        let poll = Timer(timeInterval: 0.25, repeats: true) { [weak self] _ in self?.refresh() }
        timer = poll
        RunLoop.main.add(poll, forMode: .common)
    }

    required init?(coder: NSCoder) { fatalError("init(coder:) has not been implemented") }
    deinit { timer?.invalidate() }

    private func refresh() {
        let info = LibretroCore.sharedInstance().azaharRoomStatus()
        if let lastStatus, lastStatus.isEqual(info as NSDictionary) { return }
        lastStatus = info as NSDictionary
        let supported = info["supported"] as? Bool ?? false
        let state = (info["state"] as? NSNumber)?.intValue ?? 0
        let error = (info["error"] as? NSNumber)?.intValue ?? 0
        if port.text?.isEmpty != false, let value = info["port"] as? NSNumber {
            port.text = value.stringValue // Core supplies its DefaultRoomPort.
        }
        let active = (2...4).contains(state)
        [host, port, nickname, password].forEach { $0.isEnabled = !active && supported }
        join.isEnabled = supported && state == 1
        leave.isEnabled = active && state != 4
        let states = ["ゲームの起動待ち", "未接続", "接続中（参加未確定）", "ルーム参加済み", "退出処理中"]
        if !supported {
            if Bundle.main.object(forInfoDictionaryKey: "ManicRoomBuildMode") as? String == "ui" {
                status.text = "この元コアでは利用可能な入口を確認できないため、Room接続は無効です。UI検証用・Room未対応です。"
            } else {
                status.text = "実行中コアのRoom APIを確認できません。ゲーム起動前やAPIの取得失敗時も接続は無効です。元コアのUI検証用ビルドはRoom未対応です。"
            }
        } else {
            status.text = (states.indices.contains(Int(state)) ? states[Int(state)] : "不明な状態")
            if error != 0 { status.text = (status.text ?? "") + "\n" + Self.errorText(error) + " (\(error))" }
        }
        let names = info["members"] as? [String] ?? []
        members.text = names.isEmpty ? "参加者なし" : "参加者 (\(names.count))\n" + names.joined(separator: "\n")
    }

    @objc private func connect() {
        endEditing(true)
        guard let portValue = UInt(port.text ?? ""), (1...65535).contains(portValue) else {
            status.text = "ポートは1〜65535で入力してください。"
            return
        }
        let result = LibretroCore.sharedInstance().joinAzaharRoom(
            host.text?.trimmingCharacters(in: .whitespacesAndNewlines) ?? "", port: portValue,
            nickname: nickname.text ?? "", password: password.text ?? "")
        password.text = "" // Not persisted or logged.
        if result == 0 { refresh() }
        else { status.text = result == 1 ? "IPv4・ポート・ニックネームを確認してください。パスワードはUTF-8で128バイト以内です。" : "接続要求を受け付けられませんでした (\(result))。ゲーム起動とコアの対応状況を確認してください。" }
    }

    @objc private func disconnect() {
        LibretroCore.sharedInstance().leaveAzaharRoom()
        refresh()
    }

    private static func errorText(_ error: Int) -> String {
        switch error {
        case 1: return "ホストまたはネットワークとの接続が切れました"
        case 2: return "ホストから退出させられました"
        case 4: return "ニックネームが使用中です"
        case 5: return "MACアドレスが重複しています"
        case 6: return "エミュレート本体IDが重複しています"
        case 7: return "ルームの通信プロトコルが一致しません"
        case 8: return "パスワードが違います"
        case 9: return "接続先から応答がありません。IP・ポート・Wi-Fi・権限を確認してください"
        case 10: return "ルームが満員です"
        case 11: return "ホストから参加を拒否されています"
        case 12: return "この操作の権限がありません"
        case 13: return "対象の参加者が見つかりません"
        case 101: return "ルーム参加応答がタイムアウトしました"
        default: return "通信の初期化または処理に失敗しました"
        }
    }
}

extension AzaharRoomView: ShowableView {
    static func showRoom(hideCompletion: (() -> Void)?) {
        Self.show()?.hideCompletion = hideCompletion
    }
    func didHide() {
        guard !didFinishHiding else { return }
        didFinishHiding = true
        timer?.invalidate()
        timer = nil
        password.text = ""
        // Closing while joining cancels. Closing after Joined resumes local play.
        let info = LibretroCore.sharedInstance().azaharRoomStatus()
        if (info["state"] as? NSNumber)?.intValue == 2 { LibretroCore.sharedInstance().leaveAzaharRoom() }
        hideCompletion?()
        hideCompletion = nil
    }
}
