#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TOOLS_DIR="$ROOT/../mpc-vst-plugins"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
"${CXX:-g++}" -std=gnu++14 -fsanitize=address,undefined -fno-omit-frame-pointer -g -O1 \
  -I"$TOOLS_DIR/wrapper" -I"$ROOT/src" "$ROOT/src/break_lab_90.cpp" "$ROOT/tests/sample_bank_tests.cpp" \
  -o "$TMP/sample_bank_tests"
"$TMP/sample_bank_tests"
