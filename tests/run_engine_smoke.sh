#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
CXX="${CXX:-g++}"
"$CXX" -std=gnu++11 -fsanitize=address,undefined -fno-omit-frame-pointer -g -O1 \
  -I"$ROOT/wrapper" "$ROOT/src/break_lab_90.cpp" "$ROOT/tests/engine_smoke.cpp" \
  -o "$TMP/engine_smoke"
"$TMP/engine_smoke"
