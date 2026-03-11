# Electron Icon Design

**Date:** 2026-03-11

**Goal:** Use `/Users/Code/Minisheet/frontend/public/Minisheet.png` as the single source of truth for Electron app icons on macOS and Windows.

## Scope

- Keep `frontend/public/Minisheet.png` as the only hand-maintained logo asset.
- Generate platform packaging assets under `electron/assets/`.
- Wire Electron Forge packaging to the generated assets.
- Wire the Electron main process to use the PNG source for the runtime window icon where supported.

## Chosen Approach

Use a build-time asset generation step:

- Generate `electron/assets/Minisheet.icns` on macOS from the PNG source.
- Generate `electron/assets/Minisheet.ico` from the same PNG source.
- Expose helper functions for icon source/output paths so config and tests share one path contract.
- Run icon generation from the existing `prepare:desktop` flow before packaging.

## Why This Approach

- One source image keeps logo maintenance simple.
- Generated `.icns` and `.ico` match Electron packaging expectations better than a raw PNG alone.
- The implementation fits the current Electron scripts structure without changing the app architecture.

## Verification

- Node tests for icon path helpers and forge config expectations.
- Run the icon generation script directly.
- Run Electron script tests.
- Run `npm --prefix electron run prepare:desktop` to confirm the full packaging prep path works.
