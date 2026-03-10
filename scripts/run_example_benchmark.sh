#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BACKEND_BUILD_DIR="$ROOT_DIR/backend/build"

cmake -S "$ROOT_DIR/backend" -B "$BACKEND_BUILD_DIR" >/dev/null
cmake --build "$BACKEND_BUILD_DIR" >/dev/null

"$BACKEND_BUILD_DIR/minisheet_cli" benchmark \
  "$ROOT_DIR/examples/benchmark-case-1.csv" \
  "$ROOT_DIR/examples/benchmark-case-2.csv" \
  "$ROOT_DIR/examples/benchmark-case-3.csv"

