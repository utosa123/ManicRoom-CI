#!/bin/bash
set -euo pipefail
[[ "$(uname -s)" == Darwin && $# == 1 ]] || exit 1
out="$(cd "$1" && pwd)"
binary="$out/build/bin/Release/azahar_libretro.dylib"
test -f "$binary"
fw="$out/azahar.libretro.framework"
test ! -e "$fw"

mkdir "$fw"
cp "$binary" "$fw/azahar.libretro"
install_name_tool -id '@rpath/azahar.libretro.framework/azahar.libretro' "$fw/azahar.libretro"
cat > "$fw/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
<key>CFBundleExecutable</key><string>azahar.libretro</string>
<key>CFBundleIdentifier</key><string>com.azahar.libretro</string>
<key>CFBundleName</key><string>azahar.libretro</string>
<key>CFBundlePackageType</key><string>FMWK</string>
<key>CFBundleShortVersionString</key><string>1.0.0</string>
<key>CFBundleVersion</key><string>1</string>
<key>CFBundleSupportedPlatforms</key><array><string>iPhoneOS</string></array>
<key>MinimumOSVersion</key><string>15.0</string>
</dict></plist>
PLIST
