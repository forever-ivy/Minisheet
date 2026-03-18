import { ChangeEvent, FormEvent, useCallback, useEffect, useMemo, useRef, useState } from 'react';
import { Check, Upload } from 'lucide-react';

import { fetchBlob, fetchJson, downloadBlob } from '../app/workbookApi';
import {
  BROWSER_DRAFT_KEY,
  BrowserDraft,
  buildBrowserDraft,
  buildCsvFromSnapshot,
  buildSheetSelection,
  buildSingleCellSelection,
  emptySnapshot,
  formulaButtons,
  normalizeSnapshot,
  parseBrowserDraft,
} from '../app/workbookUtils';
import { getApiBase } from '../lib/desktopRuntime';
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
} from '../spreadsheet/adapter';
import { useWorkbookShortcuts } from './useWorkbookShortcuts';

const API_BASE = getApiBase();

export function useWorkbookController() {
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
  const currentCellIdRef = useRef('A1');

  const cancelPendingFormulaAction = useCallback(() => {
    formulaSelectionArmedRef.current = false;
    setPendingFormulaAction(null);
    setPendingTargetCellId(null);
    setFormulaMode('idle');
  }, []);

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

  const visibleActions = useMemo(() => {
    const toolbarActions = {
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
    };

    return toolbarActions[activeTab];
  }, [activeTab, handleFormulaToolbar, handleSaveToBrowser]);

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

  useWorkbookShortcuts({
    pendingFormulaAction,
    cancelPendingFormulaAction,
    selectAllCells,
    activeCell,
    formulaDraft,
    formulaMode,
    formulaTargetCellId,
    snapshotMaxCols: snapshot.maxCols,
    snapshotMaxRows: snapshot.maxRows,
    formulaSelectionArmedRef,
    gridSelectionRef,
    setActiveCell,
    setGridSelection,
    setFormulaDraft,
    setFormulaMode,
    retargetCellState,
    clearSelection: () => clearSelection(),
  });

  return {
    snapshot,
    activeCell,
    gridSelection,
    formulaDraft,
    formulaInputRef,
    workbookTitle,
    setWorkbookTitle,
    activeTab,
    setActiveTab,
    helpOpen,
    openHelp: () => setHelpOpen(true),
    closeHelp: () => setHelpOpen(false),
    csvInputRef,
    datInputRef,
    handleCsvChange,
    handleDatChange,
    handleExportCsv,
    triggerDatUpload: () => datInputRef.current?.click(),
    handleExportDat,
    visibleActions,
    handleFormulaSubmit,
    handleFormulaFocus,
    handleFormulaChange,
    handleGridPointerDown,
    handleSelectionChange,
    handleSelectionFinish,
    commitCell,
    clearSelection,
    selectAllCells,
    statusLabel,
    rangeRef,
    selectionStats,
    nonEmptyCount: Object.keys(snapshot.cells).length,
  };
}
