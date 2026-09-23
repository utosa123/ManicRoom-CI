#!/bin/bash
set -euo pipefail
mkdir -p logs
exec > >(tee logs/macos-preflight.log) 2>&1
export DEVELOPER_DIR=/Applications/Xcode_26.6.app/Contents/Developer
test -d "$DEVELOPER_DIR" # No silent Xcode fallback if the hosted image changes.
xcodebuild -version
xcodebuild -showsdks
xcrun --sdk iphoneos --show-sdk-version
xcrun clang --version
xcrun swift --version
df -h .
env | grep -E '^(ImageOS|ImageVersion)=' || true
python3 -m venv .venv
.venv/bin/python -m pip install cmake==3.31.6
.venv/bin/python -m pip freeze > logs/python-tools.txt
echo "$PWD/.venv/bin" >> "$GITHUB_PATH"
echo "DEVELOPER_DIR=$DEVELOPER_DIR" >> "$GITHUB_ENV"
