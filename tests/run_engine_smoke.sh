#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TOOLS_DIR="$ROOT/../mpc-vst-plugins"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
CXX="${CXX:-g++}"
"$CXX" -std=gnu++14 -fsanitize=address,undefined -fno-omit-frame-pointer -g -O1 \
  -I"$TOOLS_DIR/wrapper" "$ROOT/src/break_lab_90.cpp" "$ROOT/tests/engine_smoke.cpp" \
  -o "$TMP/engine_smoke"
"$TMP/engine_smoke"
