# Electron Packaging Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Keep the existing Electron Forge packaging flow, continue building macOS Apple Silicon output, and add Windows installer `exe` support without changing packaging tools.

**Architecture:** The implementation stays inside the existing `electron/` workspace. We will protect the behavior with Node tests, then extend the Forge makers list and package metadata so Windows packaging can emit a Squirrel installer while macOS keeps its current arm64 archive flow.

**Tech Stack:** Electron Forge, Node.js `node:test`, npm, existing Electron helper scripts

---

### Task 1: Add a failing packaging configuration test

**Files:**
- Create: `electron/scripts/forge-makers.test.cjs`
- Modify: `electron/forge.config.js`

**Step 1: Write the failing test**

```js
const test = require('node:test');
const assert = require('node:assert/strict');

function loadForgeConfig() {
  delete require.cache[require.resolve('../forge.config.js')];
  return require('../forge.config.js');
}

test('forge config includes the Windows squirrel maker and keeps zip packaging', () => {
  const forgeConfig = loadForgeConfig();
  const makerNames = forgeConfig.makers.map((maker) => maker.name).sort();

  assert.deepEqual(makerNames, [
    '@electron-forge/maker-squirrel',
    '@electron-forge/maker-zip',
  ]);
});
```

**Step 2: Run test to verify it fails**

Run: `node --test electron/scripts/forge-makers.test.cjs`
Expected: FAIL because the current config only includes `@electron-forge/maker-zip`

**Step 3: Write minimal implementation**

Update `electron/forge.config.js` to add the Windows Squirrel maker while keeping the existing zip maker.

**Step 4: Run test to verify it passes**

Run: `node --test electron/scripts/forge-makers.test.cjs`
Expected: PASS

**Step 5: Commit**

```bash
git add electron/scripts/forge-makers.test.cjs electron/forge.config.js
git commit -m "test: cover electron forge makers"
```

### Task 2: Add the Windows installer dependency and metadata

**Files:**
- Modify: `electron/package.json`
- Modify: `electron/package-lock.json`

**Step 1: Write the failing test**

Extend the Forge maker test so it also asserts the Windows Squirrel maker is scoped to `win32`, and add a package metadata test that checks for `description` and `author`.

```js
test('desktop package metadata required for packaging is present', () => {
  const pkg = require('../package.json');

  assert.equal(typeof pkg.description, 'string');
  assert.notEqual(pkg.description.trim(), '');
  assert.equal(typeof pkg.author, 'string');
  assert.notEqual(pkg.author.trim(), '');
});
```

**Step 2: Run test to verify it fails**

Run: `node --test electron/scripts/forge-makers.test.cjs`
Expected: FAIL because `description` and `author` are currently missing

**Step 3: Write minimal implementation**

- Add `@electron-forge/maker-squirrel` to `devDependencies`
- Add the minimal package metadata required for installer packaging
- Refresh `package-lock.json` via `npm install`

**Step 4: Run test to verify it passes**

Run: `node --test electron/scripts/forge-makers.test.cjs`
Expected: PASS

**Step 5: Commit**

```bash
git add electron/package.json electron/package-lock.json electron/scripts/forge-makers.test.cjs
git commit -m "build: add windows installer packaging metadata"
```

### Task 3: Update packaging documentation and verify the build commands

**Files:**
- Modify: `README.md`

**Step 1: Write the failing test**

No automated doc test is needed. Use command verification instead.

**Step 2: Run verification commands before doc updates**

Run: `node --test electron/scripts/*.test.cjs`
Expected: PASS for the updated Electron script tests

**Step 3: Write minimal implementation**

Document:

- `make:mac` builds the Apple Silicon package
- `make:win` uses the same Forge flow and emits Windows installer artifacts
- Windows `exe` generation requires a supported host environment

**Step 4: Run verification to confirm the repo state**

Run: `node --test electron/scripts/*.test.cjs`
Expected: PASS

Run: `npm --prefix electron run make:mac`
Expected: Either a successful macOS arm64 package, or a concrete dependency error to report honestly

Run: `npm --prefix electron run make:win`
Expected: On macOS, packaging should stop with the host-platform limitation for Squirrel.Windows, which should be captured in the final report

**Step 5: Commit**

```bash
git add README.md
git commit -m "docs: clarify electron packaging targets"
```
