#!/bin/bash
# A baseline, B UI-only original core, C isolated experimental core.
# Root may be a pristine detached checkout for A; scripts need not be installed there.
set -euo pipefail
[[ "$(uname -s)" == Darwin && $# == 4 ]] || {
  echo 'Usage (macOS): build-app.sh baseline|ui|experimental REPO NEW_OUTPUT BUNDLE_ID' >&2; exit 1;
}
mode="$1"; root="$(cd "$2" && pwd)"; out="$3"; bundle="$4"
scripts="$(cd "$(dirname "$0")" && pwd)"
case "$mode" in baseline|ui|experimental) ;; *) exit 1;; esac
[[ "$bundle" == *.* && "$bundle" != com.aoshuang.manicemu && ! -e "$out" ]] || exit 1
mkdir -p "$out"; out="$(cd "$out" && pwd)"
exec > >(tee "$out/driver.log") 2>&1
binary="$root/Cores/azahar.libretro.framework/azahar.libretro"
original_sha=183159290d777d42a68c17f5f4d90d8b88f7aa0281e4788bad4e5954a6df940c
if [[ "$mode" == experimental ]]; then
  : "${MANIC_ORIGINAL_CORE:?Set to the protected original binary outside the staging checkout}"
  [[ "$(shasum -a 256 "$MANIC_ORIGINAL_CORE" | awk '{print $1}')" == "$original_sha" ]]
  bash "$scripts/check-experimental-core.sh" "$MANIC_ORIGINAL_CORE" "$binary"
else
  [[ "$(shasum -a 256 "$binary" | awk '{print $1}')" == "$original_sha" ]] || { echo 'Original core mismatch'; exit 1; }
fi
bash "$root/Scripts/VerifyLFS.sh"
{
  xcodebuild -version; xcrun --sdk iphoneos --show-sdk-version; xcrun swift --version
  git -C "$root" rev-parse HEAD; git -C "$root" submodule status --recursive
  df -h "$out"; env | grep -E '^(ImageOS|ImageVersion)=' || true
} > "$out/build-environment.txt"
git -C "$root" diff --binary > "$out/app.patch"
git -C "$root/Dependencies/Libretro" diff --binary > "$out/libretro.patch"
project="$root/ManicEmu/ManicEmu.xcodeproj"
resolved="$project/project.xcworkspace/xcshareddata/swiftpm/Package.resolved"
test -f "$resolved"
cp "$resolved" "$out/Package.resolved.before"
xcodebuild -resolvePackageDependencies -project "$project" -scheme ManicEmuSideload \
  -onlyUsePackageVersionsFromResolvedFile -clonedSourcePackagesDirPath "$out/packages" 2>&1 | tee "$out/packages.log"
cmp "$resolved" "$out/Package.resolved.before"
xcodebuild -project "$project" -scheme ManicEmuSideload -configuration SideloadRelease \
  -sdk iphoneos -destination 'generic/platform=iOS' -derivedDataPath "$out/DerivedData" \
  -clonedSourcePackagesDirPath "$out/packages" -disableAutomaticPackageResolution \
  -archivePath "$out/ManicRoom.xcarchive" APP_BUNDLE_IDENTIFIER="$bundle" \
  CODE_SIGNING_ALLOWED=NO CODE_SIGNING_REQUIRED=NO CODE_SIGN_IDENTITY='' CODE_SIGN_ENTITLEMENTS='' \
  DEVELOPMENT_TEAM='' archive 2>&1 | tee "$out/archive.log"
app="$out/ManicRoom.xcarchive/Products/Applications/ManicEmuSideload.app"
test -d "$app"
case "$mode" in
 baseline) label='Manic Baseline';;
 ui) label='Manic UI・Room未対応';;
 experimental) label='Manic Room 実験用';;
esac
/usr/libexec/PlistBuddy -c "Set :CFBundleDisplayName $label" "$app/Info.plist"
/usr/libexec/PlistBuddy -c "Add :ManicRoomBuildMode string $mode" "$app/Info.plist"
printf '%s\n' "$mode" > "$out/build-mode.txt"
printf '%s\n' "$label: unsigned; device, keyboard, JIT and game communication NOT VERIFIED" > "$out/NOTICE.txt"
echo 'Archive complete. Generate IPA separately with package-unsigned.sh.'
