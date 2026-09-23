#!/bin/bash
set -euo pipefail
[[ $# == 1 && "$(uname -s)" == Darwin ]] || exit 1
out="$(cd "$1" && pwd)"
mode="$(cat "$out/build-mode.txt")"
case "$mode" in baseline|ui|experimental) ;; *) exit 1;; esac
app="$out/ManicRoom.xcarchive/Products/Applications/ManicEmuSideload.app"
test -d "$app"; test ! -e "$out/Payload"
mkdir "$out/Payload"; ditto "$app" "$out/Payload/ManicEmuSideload.app"
(cd "$out" && /usr/bin/zip -qry "ManicRoom-${mode}-UNSIGNED.ipa" Payload)
shasum -a 256 "$out/ManicRoom-${mode}-UNSIGNED.ipa" > "$out/IPA-SHA256SUMS"
echo "UNSIGNED: $out/ManicRoom-${mode}-UNSIGNED.ipa"
