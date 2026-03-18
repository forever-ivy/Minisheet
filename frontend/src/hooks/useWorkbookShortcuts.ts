import { Dispatch, MutableRefObject, SetStateAction, useEffect } from 'react';

import {
  ActiveCell,
  FormulaMode,
  GridSelection,
  ToolbarFormulaAction,
  applySelectionToFormula,
  cellIdToGridCoords,
  gridCoordsToCellId,
} from '../spreadsheet/adapter';
import {
  buildSingleCellSelection,
  clamp,
  isTextEntryTarget,
} from '../app/workbookUtils';

type UseWorkbookShortcutsOptions = {
  pendingFormulaAction: ToolbarFormulaAction | null;
  cancelPendingFormulaAction: () => void;
  selectAllCells: () => void;
  activeCell: ActiveCell | null;
  formulaDraft: string;
  formulaMode: FormulaMode;
  formulaTargetCellId: string;
  snapshotMaxCols: number;
  snapshotMaxRows: number;
  formulaSelectionArmedRef: MutableRefObject<boolean>;
  gridSelectionRef: MutableRefObject<GridSelection>;
  setActiveCell: Dispatch<SetStateAction<ActiveCell | null>>;
  setGridSelection: Dispatch<SetStateAction<GridSelection>>;
  setFormulaDraft: Dispatch<SetStateAction<string>>;
  setFormulaMode: Dispatch<SetStateAction<FormulaMode>>;
  retargetCellState: (nextActiveCell: ActiveCell, options?: { clearPending?: boolean }) => void;
  clearSelection: () => Promise<void> | void;
};

export function useWorkbookShortcuts({
  pendingFormulaAction,
  cancelPendingFormulaAction,
  selectAllCells,
  activeCell,
  formulaDraft,
  formulaMode,
  formulaTargetCellId,
  snapshotMaxCols,
  snapshotMaxRows,
  formulaSelectionArmedRef,
  gridSelectionRef,
  setActiveCell,
  setGridSelection,
  setFormulaDraft,
  setFormulaMode,
  retargetCellState,
  clearSelection,
}: UseWorkbookShortcutsOptions) {
  useEffect(() => {
    const root = document.documentElement;
    const body = document.body;
    const previous = {
      rootOverflow: root.style.overflow,
      rootOverscroll: root.style.overscrollBehavior,
      bodyOverflow: body.style.overflow,
      bodyOverscroll: body.style.overscrollBehavior,
    };

    root.style.overflow = 'hidden';
    root.style.overscrollBehavior = 'none';
    body.style.overflow = 'hidden';
    body.style.overscrollBehavior = 'none';

    return () => {
      root.style.overflow = previous.rootOverflow;
      root.style.overscrollBehavior = previous.rootOverscroll;
      body.style.overflow = previous.bodyOverflow;
      body.style.overscrollBehavior = previous.bodyOverscroll;
    };
  }, []);

  useEffect(() => {
    const handleKeyDown = (event: KeyboardEvent) => {
      if (event.key !== 'Escape' || !pendingFormulaAction) {
        return;
      }

      event.preventDefault();
      cancelPendingFormulaAction();
    };

    document.addEventListener('keydown', handleKeyDown);
    return () => document.removeEventListener('keydown', handleKeyDown);
  }, [cancelPendingFormulaAction, pendingFormulaAction]);

  useEffect(() => {
    const handleGridShortcuts = (event: KeyboardEvent) => {
      if (isTextEntryTarget(event.target)) {
        return;
      }

      if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === 'a') {
        event.preventDefault();
        selectAllCells();
      }
    };

    document.addEventListener('keydown', handleGridShortcuts);
    return () => document.removeEventListener('keydown', handleGridShortcuts);
  }, [selectAllCells]);

  useEffect(() => {
    const handleNavigationKeyDown = (event: KeyboardEvent) => {
      const isNumpadDigitMove =
        (event.code === 'Numpad8' && event.key === '8') ||
        (event.code === 'Numpad2' && event.key === '2') ||
        (event.code === 'Numpad4' && event.key === '4') ||
        (event.code === 'Numpad6' && event.key === '6');

      if (
        (event.defaultPrevented && !isNumpadDigitMove) ||
        event.altKey ||
        event.ctrlKey ||
        event.metaKey ||
        isTextEntryTarget(event.target)
      ) {
        return;
      }

      let rowDelta = 0;
      let colDelta = 0;

      if (isNumpadDigitMove) {
        if (event.code === 'Numpad8') {
          rowDelta = -1;
        } else if (event.code === 'Numpad2') {
          rowDelta = 1;
        } else if (event.code === 'Numpad4') {
          colDelta = -1;
        } else {
          colDelta = 1;
        }
      } else if (event.key === 'ArrowUp') {
        rowDelta = -1;
      } else if (event.key === 'ArrowDown' || event.key === 'Enter') {
        rowDelta = 1;
      } else if (event.key === 'ArrowLeft') {
        colDelta = -1;
      } else if (event.key === 'ArrowRight' || event.key === 'Tab') {
        colDelta = event.shiftKey ? -1 : 1;
      } else {
        return;
      }

      event.preventDefault();

      const baseCell = activeCell ?? cellIdToGridCoords(formulaTargetCellId || 'A1');
      const nextRow = clamp(baseCell.row + rowDelta, 0, Math.max(0, snapshotMaxRows - 1));
      const nextCol = clamp(baseCell.col + colDelta, 0, Math.max(0, snapshotMaxCols - 1));
      const nextActiveCell = {
        row: nextRow,
        col: nextCol,
        cellId: gridCoordsToCellId(nextRow, nextCol),
      };

      if (formulaSelectionArmedRef.current && formulaMode !== 'idle' && formulaDraft.trim().startsWith('=')) {
        const nextSelection = buildSingleCellSelection(nextActiveCell);
        setActiveCell(nextActiveCell);
        gridSelectionRef.current = nextSelection;
        setGridSelection(nextSelection);
        setFormulaDraft((current) => applySelectionToFormula(current, nextSelection));
        setFormulaMode('range-selecting');
        return;
      }

      formulaSelectionArmedRef.current = false;
      retargetCellState(nextActiveCell, { clearPending: true });
    };

    document.addEventListener('keydown', handleNavigationKeyDown);
    return () => document.removeEventListener('keydown', handleNavigationKeyDown);
  }, [
    activeCell,
    formulaDraft,
    formulaMode,
    formulaTargetCellId,
    formulaSelectionArmedRef,
    gridSelectionRef,
    retargetCellState,
    setActiveCell,
    setFormulaDraft,
    setFormulaMode,
    setGridSelection,
    snapshotMaxCols,
    snapshotMaxRows,
  ]);

  useEffect(() => {
    const handleDeleteShortcut = (event: KeyboardEvent) => {
      if (isTextEntryTarget(event.target) || event.altKey || event.ctrlKey || event.metaKey) {
        return;
      }

      if (event.key !== 'Delete' && event.key !== 'Backspace') {
        return;
      }

      event.preventDefault();
      void clearSelection();
    };

    document.addEventListener('keydown', handleDeleteShortcut);
    return () => document.removeEventListener('keydown', handleDeleteShortcut);
  }, [clearSelection]);
}
