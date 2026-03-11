# Electron Icon Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Generate macOS and Windows Electron app icons from a single PNG source and wire them into packaging/runtime config.

**Architecture:** Add a small icon helper module and a build script under `electron/scripts/` so icon paths and generated assets are centralized. Use the existing Electron prep flow to generate `.icns` and `.ico`, then point Forge and the BrowserWindow config at those assets.

**Tech Stack:** Node.js, Electron Forge, `node:test`, macOS `sips`/`iconutil`, Python Pillow

---

### Task 1: Document the icon contract

**Files:**
- Modify: `docs/plans/2026-03-11-electron-icon-design.md`
- Create: `electron/scripts/icon-assets.cjs`
- Test: `electron/scripts/icon-assets.test.cjs`

**Step 1: Write the failing test**

Create a `node:test` suite that asserts:
- the source logo path resolves to `frontend/public/Minisheet.png`
- the generated asset paths resolve to `electron/assets/Minisheet.icns` and `electron/assets/Minisheet.ico`
- the packager icon base resolves to the icon asset path without extension

**Step 2: Run test to verify it fails**

Run: `node --test electron/scripts/icon-assets.test.cjs`
Expected: FAIL because the helper module does not exist yet.

**Step 3: Write minimal implementation**

Implement `electron/scripts/icon-assets.cjs` with shared path helpers.

**Step 4: Run test to verify it passes**

Run: `node --test electron/scripts/icon-assets.test.cjs`
Expected: PASS

### Task 2: Generate platform icon assets

**Files:**
- Create: `electron/scripts/generate-icons.cjs`
- Modify: `electron/package.json`
- Test: `electron/scripts/icon-assets.test.cjs`

**Step 1: Write the failing test**

Extend tests to assert that forge config consumes the shared icon base path and that the generation script exports callable helpers for `.ico`/`.icns` output.

**Step 2: Run test to verify it fails**

Run: `node --test electron/scripts/icon-assets.test.cjs`
Expected: FAIL because forge config and/or generation helpers are not wired yet.

**Step 3: Write minimal implementation**

Implement a generation script that:
- ensures `electron/assets/` exists
- builds a multi-size `.ico` with Pillow
- builds a `.icns` on macOS using `sips` and `iconutil`
- skips `.icns` generation on non-macOS hosts with a clear message

**Step 4: Run test to verify it passes**

Run: `node --test electron/scripts/icon-assets.test.cjs`
Expected: PASS

### Task 3: Wire packaging and runtime icon usage

**Files:**
- Modify: `electron/forge.config.js`
- Modify: `electron/main.js`
- Modify: `electron/scripts/prepare-make.cjs`
- Test: `electron/scripts/icon-assets.test.cjs`

**Step 1: Write the failing test**

Add assertions that:
- `forge.config.js` exposes `packagerConfig.icon`
- runtime icon helper path is available for the main process

**Step 2: Run test to verify it fails**

Run: `node --test electron/scripts/icon-assets.test.cjs`
Expected: FAIL because the config is not wired yet.

**Step 3: Write minimal implementation**

Wire forge packaging to the shared icon base path and set the `BrowserWindow` icon to the source PNG or generated asset path as appropriate.

**Step 4: Run test to verify it passes**

Run: `node --test electron/scripts/icon-assets.test.cjs`
Expected: PASS

### Task 4: Verify the full prep flow

**Files:**
- Verify only

**Step 1: Run generation directly**

Run: `node electron/scripts/generate-icons.cjs`
Expected: icon files are created under `electron/assets/`

**Step 2: Run Electron tests**

Run: `node --test electron/scripts/*.test.cjs`
Expected: PASS

**Step 3: Run desktop prep**

Run: `npm --prefix electron run prepare:desktop`
Expected: PASS and generated icon assets remain available for packaging
