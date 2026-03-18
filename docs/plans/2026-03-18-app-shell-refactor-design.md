# App Shell Refactor Design

**Date:** 2026-03-18

**Goal:** Reduce the size and responsibility count of `frontend/src/App.tsx` without changing spreadsheet behavior.

## Context

`frontend/src/App.tsx` has grown to nearly one thousand lines and currently mixes pure utilities, workbook state, backend synchronization, keyboard shortcuts, formula-editing flow, file import/export, and page layout rendering in one component. That makes the file hard to navigate and increases the chance of accidental regressions during future changes.

The refactor should preserve current behavior and keep the most fragile state transitions together. In particular, the active-cell refs, selection refs, and formula selection flow are coupled and should not be split across unrelated modules.

## Chosen Approach

Use one controller hook plus a few presentational components:

- Move pure helper logic into utility modules.
- Move workbook state, refs, async actions, derived values, and event handlers into `useWorkbookController`.
- Move global document effects into `useWorkbookShortcuts`.
- Extract three UI components from the page shell:
  - `AppHeader`
  - `WorkbookToolbar`
  - `FormulaBar`
- Keep `frontend/src/App.tsx` as a composition layer that wires the controller to the presentational components, the grid, the status bar, and the help modal.

## Architecture

### Controller

`frontend/src/hooks/useWorkbookController.ts` will own:

- workbook snapshot state
- active cell and selection state
- formula draft and formula mode state
- pending toolbar formula actions
- workbook title and help-modal state
- refs that protect against async state rollback
- import/export/save actions
- grid handlers
- derived UI values such as status text, range reference, visible toolbar actions, and selection stats

This keeps the state machine in one place and avoids prop-drilling implementation details across multiple components.

### Utility Modules

`frontend/src/app/workbookUtils.ts` will hold pure helpers and constants, including:

- snapshot defaults
- browser draft parsing/building
- CSV serialization
- selection helpers
- lightweight generic helpers such as `clamp`

`frontend/src/app/workbookApi.ts` will hold low-level network and download helpers used by the controller.

### Shortcut Hook

`frontend/src/hooks/useWorkbookShortcuts.ts` will contain the document-level effects for:

- scroll lock
- `Escape`
- `Ctrl`/`Cmd+A`
- arrow, tab, enter, and numpad navigation
- delete/backspace clearing

The hook will depend on controller callbacks and controller state, but it will not own workbook data.

### Presentational Components

The extracted UI components will stay stateless and receive already-shaped props:

- `AppHeader` renders brand and top-right actions.
- `WorkbookToolbar` renders tabs and toolbar actions.
- `FormulaBar` renders the current cell reference and the formula form.

Hidden file inputs will remain at the `App.tsx` level so the composition layer can own the DOM refs while child components only receive click callbacks.

## Data-Flow Constraints

The following logic must remain coordinated inside the controller:

- `currentCellIdRef`
- `gridSelectionRef`
- `formulaSelectionArmedRef`
- `retargetCellState`
- `syncSnapshot`
- `handleSelectionChange`
- `handleSelectionFinish`

These pieces protect against stale async updates and drive the formula range-selection flow. Splitting them apart would make regressions more likely.

## Verification

Because the frontend currently has no test harness configured, verification will focus on:

- TypeScript/build success via the existing frontend build
- ensuring the extracted files compile together
- preserving keyboard and selection behavior in the refactor shape

If time allows, the first candidates for future tests are the pure utility helpers, especially browser draft parsing and CSV serialization.
