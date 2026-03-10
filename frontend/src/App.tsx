import { ChangeEvent, FormEvent, useCallback, useEffect, useMemo, useRef, useState } from 'react';
import {
  Check,
  Download,
  FolderOpen,
  FunctionSquare,
  RefreshCw,
  Search,
  Sigma,
  Sparkles,
  SunMedium,
  Upload,
} from 'lucide-react';

import { HelpModal } from './components/HelpModal';
import { InsightPanel } from './components/InsightPanel';
import { StatusBar } from './components/StatusBar';
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
  buildInsightMetric,
  buildSelectionStats,
  cellIdToGridCoords,
  gridCoordsToCellId,
  gridSelectionToRangeRef,
  normalizeGridSelection,
} from './spreadsheet/adapter';

const API_BASE = 'http://127.0.0.1:8080';
const TOTAL_ROWS = 32767;
const TOTAL_COLS = 256;

const emptySnapshot: WorkbookSnapshot = {
  maxRows: TOTAL_ROWS,
  maxCols: TOTAL_COLS,
  computeMs: 0,
  cells: {},
};

const tabLabels: Array<{ value: ToolbarTab; label: string }> = [
  { value: 'home', label: '主页' },
  { value: 'insert', label: '插入' },
  { value: 'data', label: '数据' },
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
    computeMs: snapshot.computeMs || 0,
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
  const [statusText, setStatusText] = useState('正在连接服务');
  const [workbookTitle, setWorkbookTitle] = useState('未命名');
  const [activeTab, setActiveTab] = useState<ToolbarTab>('home');
  const [searchQuery, setSearchQuery] = useState('');
  const [insightsOpen, setInsightsOpen] = useState(true);
  const [pendingFormulaAction, setPendingFormulaAction] = useState<ToolbarFormulaAction | null>(null);
  const [pendingTargetCellId, setPendingTargetCellId] = useState<string | null>(null);
  const [helpOpen, setHelpOpen] = useState(false);

  const formulaInputRef = useRef<HTMLInputElement | null>(null);
  const csvInputRef = useRef<HTMLInputElement | null>(null);
  const datInputRef = useRef<HTMLInputElement | null>(null);
  const hasLoadedRef = useRef(false);
  const formulaSelectionArmedRef = useRef(false);
  const gridSelectionRef = useRef<GridSelection>(gridSelection);

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

  const syncSnapshot = useCallback((nextSnapshot: WorkbookSnapshot, preferredCellId?: string) => {
    const normalized = normalizeSnapshot(nextSnapshot);
    const nextCellId = preferredCellId ?? formulaTargetCellId ?? activeCell?.cellId ?? 'A1';
    const nextCoords = cellIdToGridCoords(nextCellId);
    const nextRaw = normalized.cells[nextCellId]?.raw ?? '';

    formulaSelectionArmedRef.current = false;
    setPendingFormulaAction(null);
    setPendingTargetCellId(null);
    setSnapshot(normalized);
    setActiveCell(nextCoords);
    const nextSelection = {
      startRow: nextCoords.row,
      startCol: nextCoords.col,
      endRow: nextCoords.row,
      endCol: nextCoords.col,
    };
    gridSelectionRef.current = nextSelection;
    setGridSelection(nextSelection);
    setFormulaTargetCellId(nextCellId);
    setFormulaDraft(nextRaw);
    setFormulaMode('idle');
    setBackendOnline(true);
    setStatusText('已自动保存');
  }, [activeCell?.cellId, formulaTargetCellId]);

  const loadSnapshot = useCallback(async () => {
    try {
      const nextSnapshot = await fetchJson<WorkbookSnapshot>(`${API_BASE}/api/snapshot`);
      syncSnapshot(nextSnapshot, formulaTargetCellId || 'A1');
    } catch (error) {
      setBackendOnline(false);
      setStatusText('服务离线');
      setFormulaMode('idle');
      console.error(error);
    }
  }, [formulaTargetCellId, syncSnapshot]);

  useEffect(() => {
    if (hasLoadedRef.current) {
      return;
    }

    hasLoadedRef.current = true;
    void loadSnapshot();
  }, [loadSnapshot]);

  async function commitCell(cellId: string, raw: string) {
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
      setStatusText('服务离线');
      console.error(error);
    }
  }

  function handleGridPointerDown() {
    formulaSelectionArmedRef.current = formulaMode !== 'idle' && formulaDraft.trim().startsWith('=');
  }

  function handleSelectionChange(selection: GridSelection, nextActiveCell: ActiveCell | null) {
    gridSelectionRef.current = selection;
    setGridSelection(selection);
    if (!nextActiveCell) {
      return;
    }

    setActiveCell(nextActiveCell);

    if (formulaSelectionArmedRef.current && formulaMode !== 'idle' && formulaDraft.trim().startsWith('=')) {
      setFormulaDraft((current) => applySelectionToFormula(current, selection));
      setFormulaMode('range-selecting');
      return;
    }

    if (formulaMode !== 'idle') {
      return;
    }

    const nextCellId = nextActiveCell.cellId;
    setFormulaTargetCellId(nextCellId);
    setFormulaDraft(snapshot.cells[nextCellId]?.raw ?? '');
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
      setStatusText('服务离线');
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

  async function handleSaveDat() {
    try {
      const blob = await fetchBlob(`${API_BASE}/api/save-dat`, {
        method: 'POST',
      });
      downloadBlob(blob, `${workbookTitle || 'workbook'}.dat`);
      setBackendOnline(true);
      setStatusText('已自动保存');
    } catch (error) {
      setBackendOnline(false);
      setStatusText('服务离线');
      console.error(error);
    }
  }

  const toolbarActions = useMemo(
    () => ({
      home: [
        { key: 'import-csv', label: '导入 CSV', icon: Upload, onClick: () => csvInputRef.current?.click() },
        { key: 'load-dat', label: '载入 DAT', icon: FolderOpen, onClick: () => datInputRef.current?.click() },
        { key: 'refresh', label: '刷新', icon: RefreshCw, onClick: () => void loadSnapshot() },
      ],
      insert: formulaButtons.map(({ kind, label, icon }) => ({
        key: kind,
        label,
        icon,
        onClick: () => handleFormulaToolbar(kind),
      })),
      data: [
        { key: 'reload', label: '重新同步', icon: RefreshCw, onClick: () => void loadSnapshot() },
        {
          key: 'insights',
          label: '切换洞察',
          icon: Sparkles,
          onClick: () => setInsightsOpen((current) => !current),
        },
      ],
    }),
    [loadSnapshot, handleFormulaToolbar],
  );

  const visibleActions = useMemo(() => {
    const query = searchQuery.trim();
    const currentActions = toolbarActions[activeTab];

    if (!query) {
      return currentActions;
    }

    return currentActions.filter((action) => action.label.includes(query));
  }, [activeTab, searchQuery, toolbarActions]);

  const insightMetric = useMemo(
    () => buildInsightMetric(snapshot, gridSelection, activeCell),
    [snapshot, gridSelection, activeCell],
  );

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
          <span>s.Sheet</span>
        </div>

        <div className="header-meta">
          <div className={`weather-widget ${backendOnline ? '' : 'is-offline'}`}>
            <SunMedium className="icon" />
            {statusText}
          </div>
          <div className="user-actions">
            <button type="button" className="help-link" onClick={() => setHelpOpen(true)}>
              帮助
            </button>
            <button
              className="icon-btn dark"
              type="button"
              aria-label="切换洞察面板"
              onClick={() => setInsightsOpen((current) => !current)}
            >
              <Sparkles className="icon" />
            </button>
            <button className="icon-btn" type="button" aria-label="刷新快照" onClick={() => void loadSnapshot()}>
              <RefreshCw className="icon" />
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

            <label className="search-pill" aria-label="搜索命令">
              <Search className="icon" />
              <input
                type="text"
                value={searchQuery}
                onChange={(event) => setSearchQuery(event.target.value)}
                placeholder="搜索命令..."
              />
            </label>

            <div className="action-pills" aria-label={`${activeTab} 工具区`}>
              {visibleActions.map(({ key, label, icon: Icon, onClick }) => (
                <button key={key} type="button" className="pill-btn" onClick={onClick}>
                  <Icon className="icon icon-small" />
                  {label}
                </button>
              ))}
            </div>

            <button className="pill-btn primary" type="button" onClick={handleSaveDat}>
              <Download className="icon icon-small" />
              导出 DAT
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
              />
            </div>

            <InsightPanel
              metric={insightMetric}
              stats={selectionStats}
              rangeRef={rangeRef}
              visible={insightsOpen}
              onToggleVisible={() => setInsightsOpen((current) => !current)}
              onExportDat={handleSaveDat}
            />
          </div>

          <StatusBar
            status={statusLabel}
            workbookTitle={workbookTitle}
            rangeRef={rangeRef}
            stats={selectionStats}
            nonEmptyCount={Object.keys(snapshot.cells).length}
            computeMs={snapshot.computeMs}
          />
        </div>
      </main>

      <HelpModal open={helpOpen} onClose={() => setHelpOpen(false)} />
    </div>
  );
}
