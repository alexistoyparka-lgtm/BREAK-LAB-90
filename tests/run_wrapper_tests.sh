#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MPC_TOOLS="${MPC_VST:-$ROOT/../mpc-vst-plugins}"
# MODULE_SUBDIR is relative to the host executable in build/, just as the MPC
# runtime resolves engine/ beside its .so. Exercise actual samples and programs.
mkdir -p "$ROOT/build/engine"
cp -R "$ROOT/engine/." "$ROOT/build/engine/"
"$MPC_TOOLS/tools/test_port.sh" "$ROOT/vst.json"
