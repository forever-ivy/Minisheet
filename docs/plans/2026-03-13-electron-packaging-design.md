# Electron Packaging Design

**Date:** 2026-03-13

## Goal

Keep the existing Electron Forge packaging workflow and extend it so the project continues to build a macOS Apple Silicon desktop package while also producing a Windows installer executable through the same `electron/` project.

## Constraints

- Preserve the current repository layout and Electron Forge-based workflow.
- Keep `make:mac` targeting `darwin/arm64`.
- Keep the Windows target at `win32/x64`.
- Avoid introducing a new packaging tool such as `electron-builder`.
- Windows installer generation must fit into the current `prepare:desktop` then `electron-forge make` flow.

## Selected Approach

Use Electron Forge makers only:

- Keep `@electron-forge/maker-zip` for the existing archive output.
- Add `@electron-forge/maker-squirrel` for Windows so `make:win` can emit a `Setup.exe`.
- Keep the existing environment-driven target selection and backend build scripts.
- Update package metadata and docs so the Windows maker has the information it expects and the packaging outputs are documented clearly.

## Trade-offs

- This preserves the original implementation style and requires the least code churn.
- Windows packaging will emit additional Squirrel artifacts (`RELEASES`, `.nupkg`) alongside the installer.
- The repository remains on Forge, so future installer customization is still more limited than a migration to `electron-builder`.

## Platform Note

Electron Forge's Squirrel.Windows maker is not supported on macOS hosts. The repository can still be prepared for Windows `exe` packaging in the existing way, but the actual `Setup.exe` build needs to run on Windows, or on a Linux environment with the required Windows packaging dependencies installed.
