#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BACKEND_BUILD_DIR="$ROOT_DIR/backend/build"
TMP_DIR="$(mktemp -d)"
SERVER_LOG="$TMP_DIR/server.log"

cleanup() {
  if [[ -n "${SERVER_PID:-}" ]]; then
    kill "$SERVER_PID" >/dev/null 2>&1 || true
    wait "$SERVER_PID" 2>/dev/null || true
  fi
  rm -rf "$TMP_DIR"
}

trap cleanup EXIT

cmake -S "$ROOT_DIR/backend" -B "$BACKEND_BUILD_DIR" >/dev/null
cmake --build "$BACKEND_BUILD_DIR" >/dev/null

"$BACKEND_BUILD_DIR/minisheet_server" >"$SERVER_LOG" 2>&1 &
SERVER_PID=$!
sleep 1

curl -sSf http://127.0.0.1:8080/api/snapshot >/dev/null
curl -sSf -X POST http://127.0.0.1:8080/api/cell \
  -H "Content-Type: application/json" \
  -d '{"cellId":"A1","raw":"12"}' >/dev/null
curl -sSf -X POST http://127.0.0.1:8080/api/save-dat >/dev/null

echo "server api smoke test passed"
