import {
  ActiveCell,
  GridSelection,
  ToolbarFormulaAction,
  ToolbarTab,
  WorkbookSnapshot,
  cellIdToGridCoords,
  gridCoordsToCellId,
} from '../spreadsheet/adapter';
import { FunctionSquare, Sigma } from 'lucide-react';

export const TOTAL_ROWS = 32767;
export const TOTAL_COLS = 256;
export const BROWSER_DRAFT_KEY = 'minisheet.browserDraft.v1';

export const emptySnapshot: WorkbookSnapshot = {
  maxRows: TOTAL_ROWS,
  maxCols: TOTAL_COLS,
  cells: {},
};

export type BrowserDraft = {
  workbookTitle: string;
  cells: Record<string, string>;
  savedAt: string;
};

export function buildSingleCellSelection(cell: ActiveCell): GridSelection {
  return {
    startRow: cell.row,
    startCol: cell.col,
    endRow: cell.row,
    endCol: cell.col,
  };
}

export function clamp(value: number, min: number, max: number): number {
  return Math.max(min, Math.min(max, value));
}

export function buildSheetSelection(maxRows: number, maxCols: number): GridSelection {
  return {
    startRow: 0,
    startCol: 0,
    endRow: Math.max(0, maxRows - 1),
    endCol: Math.max(0, maxCols - 1),
  };
}

export function isTextEntryTarget(target: EventTarget | null): boolean {
  if (!(target instanceof HTMLElement)) {
    return false;
  }

  if (target.closest('#portal')) {
    return true;
  }

  const tagName = target.tagName;
  return (
    tagName === 'INPUT' ||
    tagName === 'TEXTAREA' ||
    tagName === 'SELECT' ||
    tagName === 'BUTTON' ||
    tagName === 'A' ||
    target.isContentEditable
  );
}

export const tabLabels: Array<{ value: ToolbarTab; label: string }> = [
  { value: 'home', label: '主页' },
  { value: 'insert', label: '插入' },
];

export const formulaButtons: Array<{
  kind: ToolbarFormulaAction;
  label: string;
  icon: typeof Sigma;
}> = [
  { kind: 'sum', label: '求和', icon: Sigma },
  { kind: 'avg', label: '平均值', icon: FunctionSquare },
  { kind: 'sqrt', label: '开方', icon: FunctionSquare },
  { kind: 'abs', label: '绝对值', icon: FunctionSquare },
];

export function normalizeSnapshot(snapshot: WorkbookSnapshot): WorkbookSnapshot {
  return {
    maxRows: snapshot.maxRows || TOTAL_ROWS,
    maxCols: snapshot.maxCols || TOTAL_COLS,
    cells: snapshot.cells || {},
  };
}

function escapeCsvField(value: string) {
  if (/[",\n\r]/.test(value)) {
    return `"${value.replace(/"/g, '""')}"`;
  }

  return value;
}

export function buildCsvFromSnapshot(snapshot: WorkbookSnapshot) {
  const cells = Object.values(snapshot.cells);
  if (cells.length === 0) {
    return '';
  }

  let maxRow = 0;
  let maxCol = 0;
  for (const cell of cells) {
    const coords = cellIdToGridCoords(cell.id);
    maxRow = Math.max(maxRow, coords.row);
    maxCol = Math.max(maxCol, coords.col);
  }

  const rows: string[] = [];
  for (let row = 0; row <= maxRow; row += 1) {
    const values: string[] = [];
    for (let col = 0; col <= maxCol; col += 1) {
      const cellId = gridCoordsToCellId(row, col);
      values.push(escapeCsvField(snapshot.cells[cellId]?.raw ?? ''));
    }
    rows.push(values.join(','));
  }

  return rows.join('\n');
}

export function buildBrowserDraft(snapshot: WorkbookSnapshot, workbookTitle: string): BrowserDraft {
  const cells = Object.values(snapshot.cells).reduce<Record<string, string>>((accumulator, cell) => {
    if (cell.raw !== '') {
      accumulator[cell.id] = cell.raw;
    }
    return accumulator;
  }, {});

  return {
    workbookTitle,
    cells,
    savedAt: new Date().toISOString(),
  };
}

export function parseBrowserDraft(raw: string | null): BrowserDraft | null {
  if (!raw) {
    return null;
  }

  try {
    const parsed = JSON.parse(raw) as Partial<BrowserDraft>;
    if (typeof parsed.workbookTitle !== 'string' || typeof parsed.savedAt !== 'string') {
      return null;
    }

    if (!parsed.cells || typeof parsed.cells !== 'object' || Array.isArray(parsed.cells)) {
      return null;
    }

    const cells = Object.entries(parsed.cells).reduce<Record<string, string>>((accumulator, [cellId, value]) => {
      if (typeof value !== 'string') {
        throw new Error('invalid draft cell value');
      }
      accumulator[cellId] = value;
      return accumulator;
    }, {});

    return {
      workbookTitle: parsed.workbookTitle,
      cells,
      savedAt: parsed.savedAt,
    };
  } catch {
    return null;
  }
}
