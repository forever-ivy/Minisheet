import {
  CompactSelection,
  GridCellKind,
  type GridSelection as GlideGridSelection,
  type GridCell,
  type NumberCell,
  type Rectangle,
  type TextCell,
} from '@glideapps/glide-data-grid';

export type CellDto = {
  id: string;
  raw: string;
  display: string;
  type: string;
  error: string;
};

export type WorkbookSnapshot = {
  maxRows: number;
  maxCols: number;
  computeMs: number;
  cells: Record<string, CellDto>;
};

export type GridSelection = {
  startRow: number;
  startCol: number;
  endRow: number;
  endCol: number;
} | null;

export type ActiveCell = {
  row: number;
  col: number;
  cellId: string;
};

export type FormulaMode = 'idle' | 'editing' | 'range-selecting';
export type ToolbarFormulaAction = 'sum' | 'avg' | 'sqrt' | 'abs';
export type ToolbarTab = 'home' | 'insert' | 'data';

export type InsightMetric = {
  values: number[];
  sum: number;
  growthPct: number;
};

export type SelectionStats = {
  count: number;
  sum: number;
  avg: number;
};

const FORMULA_NAMES: Record<ToolbarFormulaAction, string> = {
  sum: 'SUM',
  avg: 'AVG',
  sqrt: 'SQRT',
  abs: 'ABS',
};

const ERROR_THEME = {
  bgCell: '#FFF7F7',
  textDark: '#B91C1C',
  textMedium: '#B91C1C',
};

const FORMULA_THEME = {
  textDark: '#064E3B',
  textMedium: '#065F46',
};

export function columnIndexToName(col: number): string {
  let value = col + 1;
  let output = '';

  while (value > 0) {
    value -= 1;
    output = String.fromCharCode(65 + (value % 26)) + output;
    value = Math.floor(value / 26);
  }

  return output;
}

export function gridCoordsToCellId(row: number, col: number): string {
  return `${columnIndexToName(col)}${row + 1}`;
}

export function cellIdToGridCoords(cellId: string): ActiveCell {
  const match = cellId.toUpperCase().match(/^([A-Z]+)(\d+)$/);
  if (!match) {
    return { row: 0, col: 0, cellId: 'A1' };
  }

  const [, colText, rowText] = match;
  let col = 0;

  for (const char of colText) {
    col = col * 26 + (char.charCodeAt(0) - 64);
  }

  return {
    row: Math.max(0, Number(rowText) - 1),
    col: Math.max(0, col - 1),
    cellId: `${colText}${rowText}`,
  };
}

export function normalizeGridSelection(selection: GridSelection): GridSelection {
  if (!selection) {
    return null;
  }

  return {
    startRow: Math.min(selection.startRow, selection.endRow),
    startCol: Math.min(selection.startCol, selection.endCol),
    endRow: Math.max(selection.startRow, selection.endRow),
    endCol: Math.max(selection.startCol, selection.endCol),
  };
}

export function gridSelectionToRangeRef(selection: GridSelection): string | null {
  const normalized = normalizeGridSelection(selection);
  if (!normalized) {
    return null;
  }

  const start = gridCoordsToCellId(normalized.startRow, normalized.startCol);
  const end = gridCoordsToCellId(normalized.endRow, normalized.endCol);
  return start === end ? start : `${start}:${end}`;
}

function gridSelectionToStartCell(selection: GridSelection): string | null {
  const normalized = normalizeGridSelection(selection);
  if (!normalized) {
    return null;
  }

  return gridCoordsToCellId(normalized.startRow, normalized.startCol);
}

export function replaceFunctionArgWithRange(base: string, reference: string): string | null {
  const trimmed = base.trim();
  const match = trimmed.match(/^=\s*([A-Z]+)\s*\([^()]*\)\s*$/i);
  if (!match) {
    return null;
  }

  const name = match[1];
  return `=${name}(${reference})`;
}

export function applySelectionToFormula(base: string, selection: GridSelection): string {
  const rangeRef = gridSelectionToRangeRef(selection);
  const startCell = gridSelectionToStartCell(selection);
  if (!rangeRef || !startCell) {
    return base;
  }

  const trimmed = base.trimEnd();
  if (!trimmed) {
    return startCell;
  }

  const functionOpenMatch = trimmed.match(/^(=\s*([A-Z]+)\s*\()$/i);
  if (functionOpenMatch) {
    const funcName = functionOpenMatch[2].toUpperCase();
    const reference = funcName === 'SUM' || funcName === 'AVG' ? rangeRef : startCell;
    return `${functionOpenMatch[1]}${reference})`;
  }

  const closedFunctionMatch = trimmed.match(/^=\s*([A-Z]+)\s*\([^()]*\)\s*$/i);
  if (closedFunctionMatch) {
    const funcName = closedFunctionMatch[1].toUpperCase();
    const reference = funcName === 'SUM' || funcName === 'AVG' ? rangeRef : startCell;
    const replaced = replaceFunctionArgWithRange(trimmed, reference);
    if (replaced) {
      return replaced;
    }
  }

  const tailRefMatch = trimmed.match(/([A-Z]+[0-9]+(?::[A-Z]+[0-9]+)?)\s*$/i);
  const reference = startCell;
  if (tailRefMatch && tailRefMatch.index != null) {
    return `${trimmed.slice(0, tailRefMatch.index)}${reference}`;
  }

  return `${trimmed}${reference}`;
}

export function buildFunctionFormula(
  kind: ToolbarFormulaAction,
  activeCellId: string,
  selection: GridSelection,
): string {
  const name = FORMULA_NAMES[kind];
  const rangeRef = gridSelectionToRangeRef(selection);
  const reference = kind === 'sum' || kind === 'avg' ? rangeRef ?? activeCellId : activeCellId;
  return `=${name}(${reference})`;
}

export function getCellDtoAt(snapshot: WorkbookSnapshot, row: number, col: number): CellDto | undefined {
  return snapshot.cells[gridCoordsToCellId(row, col)];
}

function parseNumericText(value: string): number | undefined {
  const cleaned = value.replace(/,/g, '').replace(/[￥¥$]/g, '').replace(/%$/, '').trim();
  if (!cleaned) {
    return undefined;
  }

  const parsed = Number(cleaned);
  if (!Number.isFinite(parsed)) {
    return undefined;
  }

  return value.trim().endsWith('%') ? parsed / 100 : parsed;
}

export function getNumericValue(cell: CellDto | undefined): number | undefined {
  if (!cell || cell.error) {
    return undefined;
  }

  if (cell.type === 'integer' || cell.type === 'float') {
    return parseNumericText(cell.raw || cell.display);
  }

  if (cell.type === 'formula') {
    return parseNumericText(cell.display);
  }

  return undefined;
}

export function buildGlideCell(cell: CellDto | undefined, row: number, col: number): GridCell {
  const numericValue = getNumericValue(cell);
  const isFormula = cell?.type === 'formula';

  if (!cell) {
    const emptyCell: TextCell = {
      kind: GridCellKind.Text,
      allowOverlay: true,
      data: '',
      displayData: '',
      copyData: '',
    };
    return emptyCell;
  }

  if (cell.error) {
    const errorCell: TextCell = {
      kind: GridCellKind.Text,
      allowOverlay: true,
      data: cell.raw,
      displayData: cell.display || cell.error,
      copyData: cell.display || cell.error,
      themeOverride: ERROR_THEME,
    };
    return errorCell;
  }

  if (isFormula) {
    const formulaCell: TextCell = {
      kind: GridCellKind.Text,
      allowOverlay: true,
      data: cell.raw,
      displayData: cell.display,
      copyData: cell.display,
      contentAlign: numericValue == null ? 'left' : 'right',
      themeOverride: FORMULA_THEME,
    };
    return formulaCell;
  }

  if ((cell.type === 'integer' || cell.type === 'float') && numericValue != null) {
    const numberCell: NumberCell = {
      kind: GridCellKind.Number,
      allowOverlay: true,
      data: numericValue,
      displayData: cell.display,
      copyData: cell.display,
      contentAlign: 'right',
      thousandSeparator: true,
    };
    return numberCell;
  }

  const textCell: TextCell = {
    kind: GridCellKind.Text,
    allowOverlay: true,
    data: cell.raw,
    displayData: cell.display,
    copyData: cell.display,
  };
  return textCell;
}

export function gridSelectionToGlideSelection(selection: GridSelection): GlideGridSelection {
  const normalized = normalizeGridSelection(selection);
  if (!normalized) {
    return {
      columns: CompactSelection.empty(),
      rows: CompactSelection.empty(),
    };
  }

  const range: Rectangle = {
    x: normalized.startCol,
    y: normalized.startRow,
    width: normalized.endCol - normalized.startCol + 1,
    height: normalized.endRow - normalized.startRow + 1,
  };

  return {
    columns: CompactSelection.empty(),
    rows: CompactSelection.empty(),
    current: {
      cell: [normalized.startCol, normalized.startRow],
      range,
      rangeStack: [],
    },
  };
}

export function glideSelectionToGridSelection(selection: GlideGridSelection | undefined): GridSelection {
  const current = selection?.current;
  if (!current) {
    return null;
  }

  return normalizeGridSelection({
    startRow: current.range.y,
    startCol: current.range.x,
    endRow: current.range.y + current.range.height - 1,
    endCol: current.range.x + current.range.width - 1,
  });
}

function getValuesFromSelection(snapshot: WorkbookSnapshot, selection: GridSelection): number[] {
  const normalized = normalizeGridSelection(selection);
  if (!normalized) {
    return [];
  }

  const collected: Array<{ key: number; value: number }> = [];
  for (const cell of Object.values(snapshot.cells)) {
    const coords = cellIdToGridCoords(cell.id);
    if (
      coords.row < normalized.startRow ||
      coords.row > normalized.endRow ||
      coords.col < normalized.startCol ||
      coords.col > normalized.endCol
    ) {
      continue;
    }

    const value = getNumericValue(cell);
    if (value == null) {
      continue;
    }

    const key = coords.row * snapshot.maxCols + coords.col;
    collected.push({ key, value });
  }

  collected.sort((a, b) => a.key - b.key);
  return collected.map((entry) => entry.value);
}

function getValuesFromActiveRow(snapshot: WorkbookSnapshot, activeCell: ActiveCell | null): number[] {
  if (!activeCell) {
    return [];
  }

  const values: number[] = [];
  for (const cell of Object.values(snapshot.cells)) {
    const coords = cellIdToGridCoords(cell.id);
    if (coords.row !== activeCell.row) {
      continue;
    }

    const value = getNumericValue(cell);
    if (value != null) {
      values.push(value);
    }
  }

  return values;
}

export function buildInsightMetric(
  snapshot: WorkbookSnapshot,
  selection: GridSelection,
  activeCell: ActiveCell | null,
): InsightMetric {
  const sampledValues = getValuesFromSelection(snapshot, selection);
  const allValues = sampledValues.length > 0 ? sampledValues : getValuesFromActiveRow(snapshot, activeCell);

  if (allValues.length === 0) {
    return {
      values: [0, 0, 0, 0, 0, 0],
      sum: 0,
      growthPct: 0,
    };
  }

  const first = allValues[0];
  const last = allValues[allValues.length - 1];
  const growthPct = first === 0 ? 0 : ((last - first) / Math.abs(first)) * 100;

  return {
    values: allValues.slice(0, 6),
    sum: allValues.reduce((total, value) => total + value, 0),
    growthPct,
  };
}

export function buildSelectionStats(
  snapshot: WorkbookSnapshot,
  selection: GridSelection,
  activeCell: ActiveCell | null,
): SelectionStats {
  const sampledValues = getValuesFromSelection(snapshot, selection);
  const values = sampledValues.length > 0 ? sampledValues : getValuesFromActiveRow(snapshot, activeCell);
  const count = values.length;
  const sum = values.reduce((total, value) => total + value, 0);
  return {
    count,
    sum,
    avg: count === 0 ? 0 : sum / count,
  };
}

export function formatInsightValue(value: number): string {
  return new Intl.NumberFormat('zh-CN', {
    maximumFractionDigits: 2,
  }).format(value);
}

export function formatGrowthPct(value: number): string {
  if (value === 0) {
    return '0.0%';
  }

  return `${value > 0 ? '+' : ''}${value.toFixed(1)}%`;
}

export function getCellTypeLabel(type: string): string {
  switch (type) {
    case 'integer':
      return '整数';
    case 'float':
      return '浮点数';
    case 'string':
      return '字符串';
    case 'formula':
      return '公式';
    case 'error':
      return '错误';
    default:
      return '空白';
  }
}
