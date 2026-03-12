# CLI Pack/Unpack Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add simple `pack` and `unpack` CLI modes so the project can round-trip CSV files through the custom DAT format for classroom acceptance.

**Architecture:** Extend the storage layer to preserve CSV dimensions and export workbooks back to CSV, then expose those operations through the existing CLI without breaking current `import/save/load` commands. Keep the workbook/formula logic untouched so acceptance-risk stays low.

**Tech Stack:** C++17, existing minisheet storage/workbook modules, CMake

---

### Task 1: Add a failing round-trip test

**Files:**
- Create: `backend/tests/storage_roundtrip.cpp`
- Modify: `backend/CMakeLists.txt`

**Step 1:** Add a small test executable that writes a CSV with blank cells, loads it, serializes/deserializes it, and exports it back to CSV.

**Step 2:** Build the test target and confirm it fails before storage export support exists.

### Task 2: Preserve CSV shape in DAT

**Files:**
- Modify: `backend/include/minisheet/m2_workbook.h`
- Modify: `backend/src/m6_storage.cpp`

**Step 1:** Add workbook metadata for source row/column counts.

**Step 2:** Populate that metadata in `load_csv`.

**Step 3:** Include the metadata in DAT serialization and restore it during DAT deserialization.

### Task 3: Add CSV export

**Files:**
- Modify: `backend/include/minisheet/m6_storage.h`
- Modify: `backend/src/m6_storage.cpp`

**Step 1:** Add `save_csv`.

**Step 2:** Export a rectangular CSV using stored dimensions, with CSV escaping for commas/quotes/newlines.

### Task 4: Expose CLI modes and verify

**Files:**
- Modify: `backend/app/minisheet_cli.cpp`
- Modify: `README.md`

**Step 1:** Add `pack` and `unpack` commands while keeping `save` and `load` as compatible aliases.

**Step 2:** Rebuild and run the round-trip test plus manual CLI verification.
