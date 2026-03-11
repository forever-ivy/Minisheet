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

CLI_USAGE="$("$BACKEND_BUILD_DIR/minisheet_cli" 2>&1 || true)"
if grep -q "benchmark" <<<"$CLI_USAGE"; then
  echo "benchmark command should not appear in CLI usage"
  exit 1
fi

curl -sSf http://127.0.0.1:8080/api/snapshot >/dev/null
curl -sSf -X POST http://127.0.0.1:8080/api/cell \
  -H "Content-Type: application/json" \
  -d '{"cellId":"A1","raw":"12"}' >/dev/null
curl -sSf -X POST http://127.0.0.1:8080/api/cell \
  -H "Content-Type: application/json" \
  -d '{"cellId":"A2","raw":"3"}' >/dev/null
FORMULA_RESPONSE="$(curl -sSf -X POST http://127.0.0.1:8080/api/cell \
  -H "Content-Type: application/json" \
  -d '{"cellId":"B1","raw":"=A1+A2"}')"
if ! grep -q '"B1":{.*"display":"15".*"type":"formula"' <<<"$FORMULA_RESPONSE"; then
  echo "formula result was not saved correctly"
  exit 1
fi

DAT_PATH="$TMP_DIR/workbook.dat"
curl -sSf -X POST http://127.0.0.1:8080/api/save-dat -o "$DAT_PATH"

LOAD_RESPONSE="$(curl -sSf -X POST http://127.0.0.1:8080/api/load-dat -F "dat=@$DAT_PATH")"
if ! grep -q '"B1":{.*"display":"15".*"type":"formula"' <<<"$LOAD_RESPONSE"; then
  echo "load-dat did not restore the workbook"
  exit 1
fi

RESTORE_STATUS="$(curl -s -o /dev/null -w "%{http_code}" -X POST http://127.0.0.1:8080/api/restore-browser-draft \
  -H "Content-Type: application/json" \
  -d '{"cells":{"A1":"12"}}')"
if [[ "$RESTORE_STATUS" != "404" ]]; then
  echo "restore-browser-draft should be removed, got HTTP $RESTORE_STATUS"
  exit 1
fi

echo "server api smoke test passed"
