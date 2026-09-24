#!/bin/bash
set -euo pipefail
mkdir -p logs
src=changes/azahar-experimental/src/citra_libretro
xcrun clang++ -std=c++20 -Wall -Wextra -Werror -I"$src" ci/cia_policy_tests.cpp -o logs/cia-policy-tests
logs/cia-policy-tests | tee logs/cia-policy-tests.txt
xcrun --sdk iphoneos clang -std=c11 -Werror -target arm64-apple-ios15.0 \
  -isysroot "$(xcrun --sdk iphoneos --show-sdk-path)" -I"$src" -fsyntax-only ci/cia_abi.c
cmp "$src/cia_update_api.h" changes/Libretro/pkg/apple/ManicEMU/ManicCIAABI.h
echo 'PASS C / ObjC header equality and iOS arm64 layout' | tee logs/cia-abi.txt
