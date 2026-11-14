#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TOOLS_DIR="$ROOT/../mpc-vst-plugins"
PAD_TEST_DIR="$(mktemp -d)"
trap 'rm -rf "$PAD_TEST_DIR"' EXIT
"${CXX:-g++}" -std=gnu++14 -fsanitize=address,undefined -fno-omit-frame-pointer -g -O1 \
  -I"$TOOLS_DIR/wrapper" "$ROOT/src/break_lab_90.cpp" "$ROOT/tests/test_pad_mapping.cpp" \
  -o "$PAD_TEST_DIR/test_pad_mapping"
"$PAD_TEST_DIR/test_pad_mapping"
