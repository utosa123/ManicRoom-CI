#!/bin/bash
# Experimental source build. Does not replace Manic's bundled core.
set -euo pipefail
if [[ "$(uname -s)" != Darwin ]]; then echo 'macOS + Xcode are required.' >&2; exit 1; fi
if [[ $# != 2 ]]; then echo "Usage: bash $0 AZAHAR_SOURCE NEW_OUTPUT_DIR" >&2; exit 1; fi
src="$(cd "$1" && pwd)"
out="$2"
[[ ! -e "$out" ]] || { echo 'Use a new output directory.' >&2; exit 1; }
mkdir -p "$out"
out="$(cd "$out" && pwd)"
{
  xcodebuild -version
  xcrun --sdk iphoneos --show-sdk-version
  cmake --version
  git -C "$src" rev-parse HEAD
  git -C "$src" submodule status --recursive
} > "$out/build-environment.txt"
git -C "$src" diff --binary > "$out/core-working-tree.patch"
tar -cf "$out/room-source.tar" -C "$src" src/citra_libretro/room_api.h \
  src/citra_libretro/room_session.h src/citra_libretro/room_session.cpp \
  src/citra_libretro/manic_compat.h src/citra_libretro/manic_compat.cpp
# Flags follow the candidate fork's .github/workflows/libretro.yml iOS job.
cmake -S "$src" -B "$out/build" -G Xcode \
  -DENABLE_LIBRETRO=ON -DMANIC_ROOM_EXPERIMENT=ON -DIOS=ON -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_SYSROOT=iphoneos -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DCMAKE_C_FLAGS=-DIOS -DCMAKE_CXX_FLAGS=-DIOS \
  -DCITRA_USE_PRECOMPILED_HEADERS=OFF -DENABLE_OPT=OFF \
  -DENABLE_BUILTIN_KEYBLOB=OFF -DENABLE_TESTS=OFF \
  -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO 2>&1 | tee "$out/configure.log"
cmake --build "$out/build" --target azahar_libretro --config Release --parallel 2 \
  2>&1 | tee "$out/build.log"
binary="$out/build/bin/Release/azahar_libretro.dylib"
test -f "$binary"
fw="$out/azahar.libretro.framework"
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
xcrun nm -gU "$fw/azahar.libretro" | awk '{print $NF}' | sort -u > "$out/exports.txt"
for symbol in api_version snapshot join leave; do
  grep -qx "_retro_azahar_room_$symbol" "$out/exports.txt"
done
xcrun lipo -info "$fw/azahar.libretro" > "$out/architecture.txt"
xcrun otool -L "$fw/azahar.libretro" > "$out/linked-libraries.txt"
shasum -a 256 "$fw/azahar.libretro" > "$out/SHA256SUMS"
echo "Experimental UNSIGNED framework: $fw"
echo 'Not installed. Check source provenance, existing extension APIs, dependencies and device regression first.'
