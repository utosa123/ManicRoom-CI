#!/bin/bash
# Explicit alternate-core contract. Does not claim original-core equivalence.
set -euo pipefail
[[ $# == 2 ]] || { echo 'Usage: check-experimental-core.sh ORIGINAL CANDIDATE' >&2; exit 1; }
tmp="$(mktemp -d)"
trap 'rm -f "$tmp/original" "$tmp/candidate" "$tmp/missing" "$tmp/expected"; rmdir "$tmp"' EXIT
xcrun nm -gU "$1" | awk '{print $NF}' | grep '^_retro_' | sort -u > "$tmp/original"
xcrun nm -gU "$2" | awk '{print $NF}' | sort -u > "$tmp/candidate"
comm -23 "$tmp/original" "$tmp/candidate" > "$tmp/missing"
# Deliberately unsupported: legacy version contract unknown, pre-init CIA
# storage setup unverified, remove_amiibo has no frontend declaration/caller.
# All other legacy exports remain mandatory.
printf '%s\n' _retro_azahar_extension_version _retro_azahar_install_cia _retro_azahar_remove_amiibo | sort > "$tmp/expected"
diff -u "$tmp/expected" "$tmp/missing"
for symbol in api_version snapshot join leave; do
  grep -qx "_retro_azahar_room_$symbol" "$tmp/candidate"
done
for symbol in set_keyboard_callback keyboard_input load_amiibo is_searching_amiibo; do
  grep -qx "_retro_azahar_$symbol" "$tmp/candidate"
done
grep -qx _retro_manic_experiment_api_version "$tmp/candidate"
[[ "$(xcrun lipo -archs "$2")" == arm64 ]]
xcrun vtool -show-build "$2" | grep -q 'platform IOS'
echo 'PASS experimental export/platform contract; NOT a drop-in replacement or device verification.'
