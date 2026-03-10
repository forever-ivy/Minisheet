#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BACKEND_BUILD_DIR="$ROOT_DIR/backend/build"

echo "[1/3] Configuring backend"
cmake -S "$ROOT_DIR/backend" -B "$BACKEND_BUILD_DIR"

echo "[2/3] Building backend"
cmake --build "$BACKEND_BUILD_DIR"

echo "[3/3] Starting minisheet_server on http://127.0.0.1:8080"
exec "$BACKEND_BUILD_DIR/minisheet_server"

