// Included inside LibretroCore's implementation; existing library menu owns entry.
- (void)manicCIAAlert:(NSString *)message {
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"C実験版 更新CIA"
        message:message preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil]];
    [ManicCIAPresenter() presentViewController:alert animated:YES completion:nil];
}

- (void)presentExperimentalCIAImporter {
    if (![NSThread isMainThread] || !ManicRoomExperiment()) return;
    if (self.ciaImportBusy || self.isRunning || ManicCIAHasLoadedCore()) {
        [self manicCIAAlert:@"ゲームとコアを終了してから導入してください。別の導入中は開始できません。"];
        return;
    }
    UIAlertController *confirm = [UIAlertController alertControllerWithTitle:@"更新CIAを選択"
        message:@"C実験版専用の3DS環境へインストールします。元Manic側には影響しません。\n所有するバックアップの更新CIAのみ選択してください。暗号化CIA・既存更新の上書きには対応しません。選択中のゲームとは別タイトルの更新も、そのタイトル用として導入されます。"
        preferredStyle:UIAlertControllerStyleAlert];
    [confirm addAction:[UIAlertAction actionWithTitle:@"キャンセル" style:UIAlertActionStyleCancel handler:nil]];
    [confirm addAction:[UIAlertAction actionWithTitle:@"CIAを選択" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
        if (self.ciaImportBusy || self.isRunning || ManicCIAHasLoadedCore()) return;
        UTType *type = [UTType typeWithFilenameExtension:@"cia" conformingToType:UTTypeData];
        if (!type) { [self manicCIAAlert:@"CIAファイル型を利用できません。"]; return; }
        self.ciaImportBusy = YES; self.ciaImportCancelled = NO;
        UIDocumentPickerViewController *picker = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:@[type] asCopy:YES];
        picker.allowsMultipleSelection = NO; picker.delegate = self;
        [ManicCIAPresenter() presentViewController:picker animated:YES completion:nil];
    }]];
    [ManicCIAPresenter() presentViewController:confirm animated:YES completion:nil];
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller {
    self.ciaImportBusy = NO;
}

- (void)manicCIADidEnterBackground:(NSNotification *)note {
    // Main-thread notification only sets an atomic flag. Worker owns the dylib until return.
    self.ciaImportCancelled = YES;
}

- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls {
    NSURL *url = urls.firstObject;
    if (!url || ![url.pathExtension.lowercaseString isEqualToString:@"cia"] || self.isRunning || ManicCIAHasLoadedCore()) {
        self.ciaImportBusy = NO; [self manicCIAAlert:@"CIAのみ選択できます。ゲーム停止中にやり直してください。"]; return;
    }
    // Present after the picker is dismissed; keep library/game launch blocked meanwhile.
    dispatch_async(dispatch_get_main_queue(), ^{
        UIAlertController *progress = [UIAlertController alertControllerWithTitle:@"更新CIAを導入中"
            message:@"C専用環境で検査しています。アプリを前面に保ってください。"
            preferredStyle:UIAlertControllerStyleAlert];
        [progress addAction:[UIAlertAction actionWithTitle:@"キャンセル" style:UIAlertActionStyleCancel handler:^(UIAlertAction *action) {
            self.ciaImportCancelled = YES;
        }]];
        [ManicCIAPresenter() presentViewController:progress animated:YES completion:nil];
        NSString *documents = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject;
        NSString *root = [documents stringByAppendingPathComponent:@"RoomExperiment-v1/3DS"];
        NSString *framework = [[NSBundle mainBundle] pathForResource:@"azahar.libretro" ofType:@"framework" inDirectory:@"Frameworks"];
        NSString *binary = [framework stringByAppendingPathComponent:@"azahar.libretro"];
        BOOL scoped = [url startAccessingSecurityScopedResource];
        __block UIBackgroundTaskIdentifier task = UIBackgroundTaskInvalid;
        task = [[UIApplication sharedApplication] beginBackgroundTaskWithExpirationHandler:^{ self.ciaImportCancelled = YES; }];
        dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED,0), ^{
            manic_cia_result_v1 result = {0};
            int32_t code = MCIA_UNSUPPORTED;
            dylib_t lib = binary ? dylib_load(binary.UTF8String) : NULL;
            if (lib) {
                manic_install_update_cia_v1 install = (manic_install_update_cia_v1)dylib_proc(lib,"retro_manic_install_update_cia_v1");
                if (install) code = install(root.UTF8String,url.path.UTF8String,&result,sizeof(result),ManicCIACancel,(__bridge void *)self);
                dylib_close(lib); // No unload while the synchronous API or its callback is active.
            }
            if (scoped) [url stopAccessingSecurityScopedResource];
            NSLog(@"C CIA result=%d title=%016llx version=%u contents=%u",code,(unsigned long long)result.title_id,result.title_version,result.content_count);
            dispatch_async(dispatch_get_main_queue(), ^{
                self.ciaImportBusy = NO;
                if (task != UIBackgroundTaskInvalid) [[UIApplication sharedApplication] endBackgroundTask:task];
                NSArray<NSString *> *messages = @[@"導入成功。アプリを再起動してから対象ゲームを起動してください。",
                    @"不正なCIA、内容ハッシュ不一致、または読み込み失敗です。",
                    @"未対応です。完全な復号済み更新CIAと対応C coreが必要です。ゲーム本体・DLC・暗号化・部分更新は扱いません。",
                    @"C専用保存先を確認できないため拒否しました。",
                    @"導入に失敗しました。新しい更新は公開していません。",
                    @"このタイトルの更新領域は既に存在します。上書きせず中止しました。",
                    @"ゲーム／コアが使用中です。完全に終了してやり直してください。",
                    @"キャンセルしました。新しい更新は公開していません。",
                    @"空き容量が不足しています。"];
                NSString *message = (code >= 0 && code < messages.count) ? messages[code] : @"不明なエラーです。";
                if (result.title_id) message = [message stringByAppendingFormat:@"\nTitle ID: %016llX\nTMD version: %u（ゲームの表示版とは別）",(unsigned long long)result.title_id,result.title_version];
                void (^show)(void) = ^{ [self manicCIAAlert:message]; };
                if (progress.presentingViewController) [progress dismissViewControllerAnimated:YES completion:show]; else show();
            });
        });
    });
}
