#!/bin/bash
set -euo pipefail
[[ $# == 2 ]] || { echo 'Usage: build-ui-only.sh NEW_OUTPUT YOUR_BUNDLE_ID' >&2; exit 1; }
scripts="$(cd "$(dirname "$0")" && pwd)"
bash "$scripts/build-app.sh" ui "$scripts/../.." "$1" "$2"
bash "$scripts/package-unsigned.sh" "$1"
