#!/bin/bash
# C only. Use build-ui-only.sh for B; use a disposable checkout for C.
set -euo pipefail
[[ $# == 2 ]] || { echo 'Usage: build-unsigned-ipa.sh NEW_OUTPUT YOUR_BUNDLE_ID' >&2; exit 1; }
scripts="$(cd "$(dirname "$0")" && pwd)"
bash "$scripts/build-app.sh" experimental "$scripts/../.." "$1" "$2"
bash "$scripts/package-unsigned.sh" "$1"
