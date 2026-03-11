import { ChangeEvent, FormEvent, useCallback, useEffect, useMemo, useRef, useState } from 'react';
import {
  Check,
  Download,
  FunctionSquare,
  Sigma,
  Upload,
} from 'lucide-react';

import { HelpModal } from './components/HelpModal';
import { Badge } from './components/ui/Badge';
import { StatusBar } from './components/StatusBar';
import { getApiBase } from './lib/desktopRuntime';
import { GreenGlideGrid } from './spreadsheet/GreenGlideGrid';
import {
  ActiveCell,
  FormulaMode,
  GridSelection,
  ToolbarFormulaAction,
  ToolbarTab,
  WorkbookSnapshot,
  applySelectionToFormula,
  buildFunctionFormula,
  buildSelectionStats,
  cellIdToGridCoords,
  gridCoordsToCellId,
  gridSelectionToRangeRef,
  normalizeGridSelection,
} from './spreadsheet/adapter';

const API_BASE = getApiBase();
const TOTAL_ROWS = 32767;
const TOTAL_COLS = 256;
const BROWSER_DRAFT_KEY = 'minisheet.browserDraft.v1';

const emptySnapshot: WorkbookSnapshot = {
  maxRows: TOTAL_ROWS,
  maxCols: TOTAL_COLS,
  cells: {},
};

type BrowserDraft = {
  workbookTitle: string;
  cells: Record<string, string>;
  savedAt: string;
};

function buildSingleCellSelection(cell: ActiveCell): GridSelection {
  return {
    startRow: cell.row,
    startCol: cell.col,
    endRow: cell.row,
    endCol: cell.col,
  };
}

function clamp(value: number, min: number, max: number): number {
  return Math.max(min, Math.min(max, value));
}

function buildSheetSelection(maxRows: number, maxCols: number): GridSelection {
  return {
    startRow: 0,
    startCol: 0,
    endRow: Math.max(0, maxRows - 1),
    endCol: Math.max(0, maxCols - 1),
  };
}

function isTextEntryTarget(target: EventTarget | null): boolean {
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

const tabLabels: Array<{ value: ToolbarTab; label: string }> = [
  { value: 'home', label: '主页' },
  { value: 'insert', label: '插入' },
];

const formulaButtons: Array<{
  kind: ToolbarFormulaAction;
  label: string;
  icon: typeof Sigma;
}> = [
  { kind: 'sum', label: '求和', icon: Sigma },
  { kind: 'avg', label: '平均值', icon: FunctionSquare },
  { kind: 'sqrt', label: '开方', icon: FunctionSquare },
  { kind: 'abs', label: '绝对值', icon: FunctionSquare },
];

function normalizeSnapshot(snapshot: WorkbookSnapshot): WorkbookSnapshot {
  return {
    maxRows: snapshot.maxRows || TOTAL_ROWS,
    maxCols: snapshot.maxCols || TOTAL_COLS,
    cells: snapshot.cells || {},
  };
}

async function fetchJson<T>(input: RequestInfo, init?: RequestInit): Promise<T> {
  const response = await fetch(input, init);
  if (!response.ok) {
    throw new Error(await response.text());
  }
  return (await response.json()) as T;
}

async function fetchBlob(input: RequestInfo, init?: RequestInit): Promise<Blob> {
  const response = await fetch(input, init);
  if (!response.ok) {
    throw new Error(await response.text());
  }
  return response.blob();
}

function downloadBlob(blob: Blob, name: string) {
  const url = window.URL.createObjectURL(blob);
  const link = document.createElement('a');
  link.href = url;
  link.download = name;
  link.click();
  window.URL.revokeObjectURL(url);
}

function escapeCsvField(value: string) {
  if (/[",\n\r]/.test(value)) {
    return `"${value.replace(/"/g, '""')}"`;
  }

  return value;
}

function buildCsvFromSnapshot(snapshot: WorkbookSnapshot) {
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

function buildBrowserDraft(snapshot: WorkbookSnapshot, workbookTitle: string): BrowserDraft {
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

function parseBrowserDraft(raw: string | null): BrowserDraft | null {
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

export default function App() {
  const [snapshot, setSnapshot] = useState<WorkbookSnapshot>(emptySnapshot);
  const [activeCell, setActiveCell] = useState<ActiveCell | null>(cellIdToGridCoords('A1'));
  const [gridSelection, setGridSelection] = useState<GridSelection>({
    startRow: 0,
    startCol: 0,
    endRow: 0,
    endCol: 0,
  });
  const [formulaTargetCellId, setFormulaTargetCellId] = useState('A1');
  const [formulaDraft, setFormulaDraft] = useState('');
  const [formulaMode, setFormulaMode] = useState<FormulaMode>('idle');
  const [backendOnline, setBackendOnline] = useState(false);
  const [workbookTitle, setWorkbookTitle] = useState('未命名');
  const [activeTab, setActiveTab] = useState<ToolbarTab>('home');
  const [pendingFormulaAction, setPendingFormulaAction] = useState<ToolbarFormulaAction | null>(null);
  const [pendingTargetCellId, setPendingTargetCellId] = useState<string | null>(null);
  const [helpOpen, setHelpOpen] = useState(false);

  const formulaInputRef = useRef<HTMLInputElement | null>(null);
  const csvInputRef = useRef<HTMLInputElement | null>(null);
  const datInputRef = useRef<HTMLInputElement | null>(null);
  const hasLoadedRef = useRef(false);
  const formulaSelectionArmedRef = useRef(false);
  const gridSelectionRef = useRef<GridSelection>(gridSelection);
  // 选中单元格的“即时来源”，用于抵抗异步回包/批量事件导致的状态回退。
  const currentCellIdRef = useRef('A1');

  const retargetCellState = useCallback((
    nextActiveCell: ActiveCell,
    options?: {
      selection?: GridSelection;
      snapshotOverride?: WorkbookSnapshot;
      clearPending?: boolean;
      formulaMode?: FormulaMode;
    },
  ) => {
    const nextSelection = options?.selection ?? buildSingleCellSelection(nextActiveCell);
    const nextSnapshot = options?.snapshotOverride ?? snapshot;

    currentCellIdRef.current = nextActiveCell.cellId;
    setActiveCell(nextActiveCell);
    gridSelectionRef.current = nextSelection;
    setGridSelection(nextSelection);
    setFormulaTargetCellId(nextActiveCell.cellId);
    setFormulaDraft(nextSnapshot.cells[nextActiveCell.cellId]?.raw ?? '');
    setFormulaMode(options?.formulaMode ?? 'idle');

    if (options?.clearPending ?? true) {
      setPendingFormulaAction(null);
      setPendingTargetCellId(null);
    }
  }, [snapshot]);

  const selectAllCells = useCallback(() => {
    const nextSelection = buildSheetSelection(snapshot.maxRows, snapshot.maxCols);
    formulaSelectionArmedRef.current = false;
    gridSelectionRef.current = nextSelection;
    setGridSelection(nextSelection);
    setFormulaMode('idle');
  }, [snapshot.maxCols, snapshot.maxRows]);

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
      if (event.key !== 'Escape') {
        return;
      }

      if (!pendingFormulaAction) {
        return;
      }

      event.preventDefault();
      formulaSelectionArmedRef.current = false;
      setPendingFormulaAction(null);
      setPendingTargetCellId(null);
      setFormulaMode('idle');
    };

    document.addEventListener('keydown', handleKeyDown);
    return () => document.removeEventListener('keydown', handleKeyDown);
  }, [pendingFormulaAction]);

  useEffect(() => {
    const handleGridShortcuts = (event: KeyboardEvent) => {
      if (isTextEntryTarget(event.target)) {
        return;
      }

      if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === 'a') {
        event.preventDefault();
        selectAllCells();
        return;
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

      const key = event.key;
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
      } else if (key === 'ArrowUp') {
        rowDelta = -1;
      } else if (key === 'ArrowDown' || key === 'Enter') {
        rowDelta = 1;
      } else if (key === 'ArrowLeft') {
        colDelta = -1;
      } else if (key === 'ArrowRight' || key === 'Tab') {
        colDelta = event.shiftKey ? -1 : 1;
      } else {
        return;
      }

      event.preventDefault();

      const baseCell = activeCell ?? cellIdToGridCoords(formulaTargetCellId || 'A1');
      const nextRow = clamp(baseCell.row + rowDelta, 0, Math.max(0, snapshot.maxRows - 1));
      const nextCol = clamp(baseCell.col + colDelta, 0, Math.max(0, snapshot.maxCols - 1));
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
  }, [activeCell, formulaDraft, formulaMode, formulaTargetCellId, retargetCellState, snapshot.maxCols, snapshot.maxRows]);

  const syncSnapshot = useCallback((nextSnapshot: WorkbookSnapshot, preferredCellId?: string) => {
    const normalized = normalizeSnapshot(nextSnapshot);
    const currentCellId = currentCellIdRef.current || activeCell?.cellId || formulaTargetCellId || 'A1';
    const nextCellId = preferredCellId && preferredCellId === currentCellId ? preferredCellId : currentCellId;
    const nextCoords = cellIdToGridCoords(nextCellId);
    const nextRaw = normalized.cells[nextCellId]?.raw ?? '';

    formulaSelectionArmedRef.current = false;
    setPendingFormulaAction(null);
    setPendingTargetCellId(null);
    setSnapshot(normalized);
    currentCellIdRef.current = nextCoords.cellId;
    setActiveCell(nextCoords);
    const nextSelection = buildSingleCellSelection(nextCoords);
    gridSelectionRef.current = nextSelection;
    setGridSelection(nextSelection);
    setFormulaTargetCellId(nextCellId);
    setFormulaDraft(nextRaw);
    setFormulaMode('idle');
    setBackendOnline(true);
  }, [activeCell?.cellId, formulaTargetCellId]);

  const loadSnapshot = useCallback(async () => {
    try {
      const nextSnapshot = await fetchJson<WorkbookSnapshot>(`${API_BASE}/api/snapshot`);
      syncSnapshot(nextSnapshot, formulaTargetCellId || 'A1');
    } catch (error) {
      setBackendOnline(false);
      setFormulaMode('idle');
      console.error(error);
    }
  }, [formulaTargetCellId, syncSnapshot]);

  const replayDraftToBackend = useCallback(async (cells: Record<string, string>) => {
    let lastSnapshot = await fetchJson<WorkbookSnapshot>(`${API_BASE}/api/snapshot`);
    const draftCellIds = new Set(Object.keys(cells));

    for (const [cellId, raw] of Object.entries(cells)) {
      lastSnapshot = await fetchJson<WorkbookSnapshot>(`${API_BASE}/api/cell`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ cellId, raw }),
      });
    }

    for (const cellId of Object.keys(lastSnapshot.cells)) {
      if (draftCellIds.has(cellId)) {
        continue;
      }

      lastSnapshot = await fetchJson<WorkbookSnapshot>(`${API_BASE}/api/cell`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ cellId, raw: '' }),
      });
    }

    return lastSnapshot;
  }, []);

  const restoreBrowserDraft = useCallback(async (draft: BrowserDraft) => {
    const nextSnapshot = await replayDraftToBackend(draft.cells);
    setWorkbookTitle(draft.workbookTitle || '未命名');
    syncSnapshot(nextSnapshot, 'A1');
  }, [replayDraftToBackend, syncSnapshot]);

  const loadInitialWorkbook = useCallback(async () => {
    const draft = parseBrowserDraft(window.sessionStorage.getItem(BROWSER_DRAFT_KEY));
    if (!draft) {
      window.sessionStorage.removeItem(BROWSER_DRAFT_KEY);
      await loadSnapshot();
      return;
    }

    try {
      await restoreBrowserDraft(draft);
    } catch (error) {
      console.error(error);
      await loadSnapshot();
    }
  }, [loadSnapshot, restoreBrowserDraft]);

  useEffect(() => {
    if (hasLoadedRef.current) {
      return;
    }

    hasLoadedRef.current = true;
    void loadInitialWorkbook();
  }, [loadInitialWorkbook]);

  const commitCell = useCallback(async (cellId: string, raw: string) => {
    try {
      const nextSnapshot = await fetchJson<WorkbookSnapshot>(`${API_BASE}/api/cell`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ cellId, raw }),
      });
      syncSnapshot(nextSnapshot, cellId);
    } catch (error) {
      setBackendOnline(false);
      console.error(error);
    }
  }, [syncSnapshot]);

  const clearSelection = useCallback(async (selectionToClear?: GridSelection) => {
    const normalized = normalizeGridSelection(selectionToClear ?? gridSelectionRef.current);
    const selectedCells = normalized
      ? Object.values(snapshot.cells)
          .filter((cell) => {
            const coords = cellIdToGridCoords(cell.id);
            return (
              coords.row >= normalized.startRow &&
              coords.row <= normalized.endRow &&
              coords.col >= normalized.startCol &&
              coords.col <= normalized.endCol
            );
          })
          .map((cell) => cell.id)
      : [];

    if (selectedCells.length === 0) {
      const fallbackCellId = currentCellIdRef.current || activeCell?.cellId;
      if (!fallbackCellId) {
        return;
      }
      await commitCell(fallbackCellId, '');
      return;
    }

    try {
      let lastSnapshot: WorkbookSnapshot | null = null;
      for (const cellId of selectedCells) {
        lastSnapshot = await fetchJson<WorkbookSnapshot>(`${API_BASE}/api/cell`, {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
          },
          body: JSON.stringify({ cellId, raw: '' }),
        });
      }

      if (lastSnapshot) {
        syncSnapshot(lastSnapshot);
      }
    } catch (error) {
      setBackendOnline(false);
      console.error(error);
    }
  }, [activeCell?.cellId, commitCell, snapshot.cells, syncSnapshot]);

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

  function handleGridPointerDown() {
    formulaInputRef.current?.blur();
    formulaSelectionArmedRef.current = formulaMode !== 'idle' && formulaDraft.trim().startsWith('=');
  }

  function handleSelectionChange(selection: GridSelection, nextActiveCell: ActiveCell | null) {
    gridSelectionRef.current = selection;
    setGridSelection(selection);
    if (!nextActiveCell) {
      return;
    }

    currentCellIdRef.current = nextActiveCell.cellId;
    setActiveCell(nextActiveCell);

    if (formulaSelectionArmedRef.current && formulaMode !== 'idle' && formulaDraft.trim().startsWith('=')) {
      setFormulaDraft((current) => applySelectionToFormula(current, selection));
      setFormulaMode('range-selecting');
      return;
    }

    if (formulaMode !== 'idle') {
      if (!formulaDraft.trim().startsWith('=')) {
        formulaSelectionArmedRef.current = false;
        retargetCellState(nextActiveCell, { selection, clearPending: true });
      }
      return;
    }

    retargetCellState(nextActiveCell, { selection, clearPending: false });
  }

  function handleSelectionFinish() {
    if (!formulaSelectionArmedRef.current) {
      return;
    }

    formulaSelectionArmedRef.current = false;

    if (pendingFormulaAction && pendingTargetCellId) {
      const normalized = normalizeGridSelection(gridSelectionRef.current);
      const startCellId = normalized
        ? gridCoordsToCellId(normalized.startRow, normalized.startCol)
        : activeCell?.cellId ?? pendingTargetCellId;
      const raw = buildFunctionFormula(pendingFormulaAction, startCellId, gridSelectionRef.current);

      setPendingFormulaAction(null);
      setPendingTargetCellId(null);
      setFormulaMode('idle');
      void commitCell(pendingTargetCellId, raw);
      return;
    }

    setFormulaMode((current) => (current === 'range-selecting' ? 'editing' : current));
  }

  const handleFormulaToolbar = useCallback((kind: ToolbarFormulaAction) => {
    const targetCellId = activeCell?.cellId ?? formulaTargetCellId ?? 'A1';
    setPendingFormulaAction(kind);
    setPendingTargetCellId(targetCellId);
    setFormulaTargetCellId(targetCellId);
    setFormulaDraft(buildFunctionFormula(kind, targetCellId, gridSelection));
    setFormulaMode('editing');
  }, [activeCell?.cellId, formulaTargetCellId, gridSelection]);

  async function handleFormulaSubmit(event?: FormEvent) {
    event?.preventDefault();
    const targetCellId = formulaTargetCellId || activeCell?.cellId || 'A1';
    setPendingFormulaAction(null);
    setPendingTargetCellId(null);
    await commitCell(targetCellId, formulaDraft);
  }

  function handleFormulaChange(event: ChangeEvent<HTMLInputElement>) {
    setFormulaDraft(event.target.value);
    setFormulaMode('editing');
    if (pendingFormulaAction) {
      setPendingFormulaAction(null);
      setPendingTargetCellId(null);
    }
    if (!formulaTargetCellId) {
      setFormulaTargetCellId(activeCell?.cellId ?? 'A1');
    }
  }

  function handleFormulaFocus() {
    setFormulaMode('editing');
    if (!formulaTargetCellId) {
      setFormulaTargetCellId(activeCell?.cellId ?? 'A1');
    }
  }

  async function uploadWorkbook(file: File, field: 'csv' | 'dat', endpoint: string) {
    const formData = new FormData();
    formData.append(field, file);

    try {
      const nextSnapshot = await fetchJson<WorkbookSnapshot>(`${API_BASE}${endpoint}`, {
        method: 'POST',
        body: formData,
      });
      setWorkbookTitle(file.name.replace(/\.[^.]+$/, '') || '未命名');
      syncSnapshot(nextSnapshot, 'A1');
    } catch (error) {
      setBackendOnline(false);
      console.error(error);
    }
  }

  async function handleCsvChange(event: ChangeEvent<HTMLInputElement>) {
    const file = event.target.files?.[0];
    if (file) {
      await uploadWorkbook(file, 'csv', '/api/import-csv');
    }
    event.target.value = '';
  }

  async function handleDatChange(event: ChangeEvent<HTMLInputElement>) {
    const file = event.target.files?.[0];
    if (file) {
      await uploadWorkbook(file, 'dat', '/api/load-dat');
    }
    event.target.value = '';
  }

  function handleExportCsv() {
    const csvContent = buildCsvFromSnapshot(snapshot);
    const blob = new Blob([csvContent], { type: 'text/csv;charset=utf-8' });
    downloadBlob(blob, `${workbookTitle || 'workbook'}.csv`);
  }

  async function handleExportDat() {
    try {
      const blob = await fetchBlob(`${API_BASE}/api/save-dat`, {
        method: 'POST',
      });
      downloadBlob(blob, `${workbookTitle || 'workbook'}.dat`);
      setBackendOnline(true);
    } catch (error) {
      setBackendOnline(false);
      console.error(error);
    }
  }

  const handleSaveToBrowser = useCallback(() => {
    const draft = buildBrowserDraft(snapshot, workbookTitle || '未命名');
    window.sessionStorage.setItem(BROWSER_DRAFT_KEY, JSON.stringify(draft));
  }, [snapshot, workbookTitle]);

  const toolbarActions = useMemo(
    () => ({
      home: [
        { key: 'import-csv', label: '导入 CSV', icon: Upload, onClick: () => csvInputRef.current?.click() },
        { key: 'save', label: '保存', icon: Check, onClick: handleSaveToBrowser },
      ],
      insert: formulaButtons.map(({ kind, label, icon }) => ({
        key: kind,
        label,
        icon,
        onClick: () => handleFormulaToolbar(kind),
      })),
    }),
    [handleFormulaToolbar, handleSaveToBrowser],
  );

  const visibleActions = useMemo(() => {
    const currentActions = toolbarActions[activeTab];
    return currentActions;
  }, [activeTab, toolbarActions]);

  const selectionStats = useMemo(
    () => buildSelectionStats(snapshot, gridSelection, activeCell),
    [snapshot, gridSelection, activeCell],
  );

  const rangeRef = useMemo(() => {
    const ref = gridSelectionToRangeRef(gridSelection);
    return ref ?? activeCell?.cellId ?? 'A1';
  }, [gridSelection, activeCell?.cellId]);

  const statusLabel = useMemo(() => {
    if (!backendOnline) {
      return '服务离线';
    }
    if (pendingFormulaAction) {
      return '等待选择';
    }
    if (formulaMode === 'range-selecting') {
      return '选择中';
    }
    if (formulaMode === 'editing') {
      return '编辑中';
    }
    return '就绪';
  }, [backendOnline, pendingFormulaAction, formulaMode]);

  return (
    <div className="green-app-shell">
      <input
        ref={csvInputRef}
        id="csv-upload"
        name="csv"
        className="hidden-input"
        type="file"
        accept=".csv,text/csv"
        onChange={handleCsvChange}
      />
      <input
        ref={datInputRef}
        id="dat-upload"
        name="dat"
        className="hidden-input"
        type="file"
        accept=".dat,application/octet-stream"
        onChange={handleDatChange}
      />

      <header className="app-header">
        <div className="brand">
          <div className="brand-icon" />
          <span>MiniSheet</span>
        </div>

        <div className="header-meta">
          <div className="user-actions">
            <button type="button" className="badge-action" onClick={() => setHelpOpen(true)}>
              <Badge variant="secondary">帮助</Badge>
            </button>
            <button type="button" className="badge-action" onClick={() => datInputRef.current?.click()}>
              <Badge variant="secondary">载入 DAT</Badge>
            </button>
            <button type="button" className="badge-action" onClick={() => void handleExportDat()}>
              <Badge variant="secondary">导出 DAT</Badge>
            </button>
          </div>
        </div>
      </header>

      <main className="workspace">
        <div className="toolbar-container">
          <div className="pill-row">
            <div className="pill-group">
              {tabLabels.map((tab) => (
                <button
                  key={tab.value}
                  type="button"
                  className={`pill-btn ${activeTab === tab.value ? 'active' : ''}`}
                  onClick={() => setActiveTab(tab.value)}
                >
                  {tab.label}
                </button>
              ))}
            </div>

            <div className="action-pills" aria-label={`${activeTab} 工具区`}>
              {visibleActions.map(({ key, label, icon: Icon, onClick }) => (
                <button key={key} type="button" className="pill-btn" onClick={onClick}>
                  <Icon className="icon icon-small" />
                  {label}
                </button>
              ))}
            </div>

            <button className="pill-btn primary" type="button" onClick={handleExportCsv}>
              <Download className="icon icon-small" />
              导出 CSV
            </button>
          </div>

          <div className="formula-bar">
            <div className="cell-ref">{activeCell?.cellId ?? 'A1'}</div>
            <div className="fx-icon">fx</div>
            <form className="formula-form" onSubmit={handleFormulaSubmit}>
              <input
                ref={formulaInputRef}
                className="formula-input"
                type="text"
                aria-label="公式栏"
                value={formulaDraft}
                onFocus={handleFormulaFocus}
                onChange={handleFormulaChange}
                placeholder="输入内容或公式"
              />
              <button type="submit" className="formula-submit" aria-label="提交公式">
                <Check className="icon icon-small" />
              </button>
            </form>
          </div>
        </div>

        <div className="sheet-container">
          <div className="grid-layer">
            <div className="grid-viewport">
              <GreenGlideGrid
                snapshot={snapshot}
                activeCell={activeCell}
                selection={gridSelection}
                onGridPointerDown={handleGridPointerDown}
                onSelectionChange={handleSelectionChange}
                onSelectionFinish={handleSelectionFinish}
                onCellCommit={commitCell}
                onDeleteSelection={clearSelection}
                onSelectAll={selectAllCells}
              />
            </div>
          </div>

          <StatusBar
            status={statusLabel}
            workbookTitle={workbookTitle}
            onWorkbookTitleChange={setWorkbookTitle}
            rangeRef={rangeRef}
            stats={selectionStats}
            nonEmptyCount={Object.keys(snapshot.cells).length}
          />
        </div>
      </main>

      <HelpModal open={helpOpen} onClose={() => setHelpOpen(false)} />
    </div>
  );
}
