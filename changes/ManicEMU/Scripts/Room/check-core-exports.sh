#!/bin/bash
set -euo pipefail
if [[ $# != 2 ]]; then echo "Usage: bash $0 ORIGINAL_CORE_BINARY CANDIDATE_CORE_BINARY" >&2; exit 1; fi
tmp="$(mktemp -d)"
# Only files created in this invocation are removed.
trap 'rm -f "$tmp/original" "$tmp/candidate" "$tmp/missing"; rmdir "$tmp"' EXIT
xcrun nm -gU "$1" | awk '{print $NF}' | grep '^_retro_' | sort -u > "$tmp/original"
xcrun nm -gU "$2" | awk '{print $NF}' | sort -u > "$tmp/candidate"
comm -23 "$tmp/original" "$tmp/candidate" > "$tmp/missing"
if [[ -s "$tmp/missing" ]]; then
  echo 'Existing Manic core APIs are missing. Do not replace its framework:' >&2
  cat "$tmp/missing" >&2
  exit 1
fi
for symbol in api_version snapshot join leave; do
  grep -qx "_retro_azahar_room_$symbol" "$tmp/candidate"
done
echo 'Export names pass; ABI semantics, core provenance, rendering/input/JIT and devices still require verification.'
