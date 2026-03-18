# App Shell Refactor Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Split `frontend/src/App.tsx` into a controller hook, utility modules, and small presentational components while preserving current spreadsheet behavior.

**Architecture:** Move pure helpers into `frontend/src/app`, move workbook orchestration into `frontend/src/hooks/useWorkbookController.ts`, move document-level effects into `frontend/src/hooks/useWorkbookShortcuts.ts`, and reduce `frontend/src/App.tsx` to a composition shell. Keep fragile selection and formula state transitions together in the controller.

**Tech Stack:** React 18, TypeScript, `react-scripts`, `lucide-react`, Glide Data Grid

---

### Task 1: Extract pure workbook utilities

**Files:**
- Create: `frontend/src/app/workbookUtils.ts`
- Create: `frontend/src/app/workbookApi.ts`
- Modify: `frontend/src/App.tsx`

**Step 1: Write the failing test**

There is no frontend test harness in this repo today. Use the TypeScript and CRA build as the verification boundary for this refactor task.

**Step 2: Run verification to establish the current baseline**

Run: `npm run build`
Working directory: `frontend`
Expected: the frontend builds successfully before and after utility extraction

**Step 3: Write minimal implementation**

- Move pure helpers and constants out of `App.tsx` into `workbookUtils.ts`
- Move fetch/blob/download helpers out of `App.tsx` into `workbookApi.ts`
- Update `App.tsx` imports to consume the extracted utilities

**Step 4: Run verification**

Run: `npm run build`
Working directory: `frontend`
Expected: successful build with unchanged behavior

### Task 2: Extract workbook controller and shortcut hook

**Files:**
- Create: `frontend/src/hooks/useWorkbookController.ts`
- Create: `frontend/src/hooks/useWorkbookShortcuts.ts`
- Modify: `frontend/src/App.tsx`

**Step 1: Write the failing test**

No test harness exists for hook-level behavior, so use the build as the red/green gate and keep the extraction behavior-preserving.

**Step 2: Run verification to establish the baseline**

Run: `npm run build`
Working directory: `frontend`
Expected: current code builds before the hook extraction

**Step 3: Write minimal implementation**

- Move workbook state, refs, actions, selection handlers, formula handlers, import/export handlers, and derived values into `useWorkbookController`
- Move document-level effects into `useWorkbookShortcuts`
- Keep the anti-race refs and selection/formula transition logic together inside the controller
- Return a stable surface from the controller for `App.tsx` consumption

**Step 4: Run verification**

Run: `npm run build`
Working directory: `frontend`
Expected: successful build after the hook extraction

### Task 3: Extract presentational app-shell components

**Files:**
- Create: `frontend/src/components/AppHeader.tsx`
- Create: `frontend/src/components/WorkbookToolbar.tsx`
- Create: `frontend/src/components/FormulaBar.tsx`
- Modify: `frontend/src/App.tsx`

**Step 1: Write the failing test**

Use the existing build as the verification gate because no component test harness is configured.

**Step 2: Run verification to establish the baseline**

Run: `npm run build`
Working directory: `frontend`
Expected: current code builds before component extraction

**Step 3: Write minimal implementation**

- Extract stateless UI components for the header, toolbar, and formula bar
- Pass already-shaped props and callbacks instead of refs when possible
- Leave hidden file inputs in `App.tsx`

**Step 4: Run verification**

Run: `npm run build`
Working directory: `frontend`
Expected: successful build with a much smaller `App.tsx`

### Task 4: Final verification and cleanup

**Files:**
- Modify: `frontend/src/App.tsx`
- Modify: `frontend/src/app/workbookApi.ts`
- Modify: `frontend/src/app/workbookUtils.ts`
- Modify: `frontend/src/hooks/useWorkbookController.ts`
- Modify: `frontend/src/hooks/useWorkbookShortcuts.ts`
- Modify: `frontend/src/components/AppHeader.tsx`
- Modify: `frontend/src/components/WorkbookToolbar.tsx`
- Modify: `frontend/src/components/FormulaBar.tsx`

**Step 1: Review the final composition**

- Ensure `App.tsx` is now a composition layer
- Ensure no duplicate logic remains in both the controller and the shell

**Step 2: Run verification**

Run: `npm run build`
Working directory: `frontend`
Expected: successful production build

**Step 3: Inspect the worktree state**

Run: `git status --short`
Expected: only the intended refactor files are changed

**Step 4: Commit**

Do not commit automatically in this repo unless the user asks, because the workspace already contains unrelated local changes.
