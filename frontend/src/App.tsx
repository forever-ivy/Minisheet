import { FormulaBar } from './components/FormulaBar';
import { HelpModal } from './components/HelpModal';
import { AppHeader } from './components/AppHeader';
import { StatusBar } from './components/StatusBar';
import { WorkbookToolbar } from './components/WorkbookToolbar';
import { useWorkbookController } from './hooks/useWorkbookController';
import { GreenGlideGrid } from './spreadsheet/GreenGlideGrid';

export default function App() {
  const {
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
    openHelp,
    closeHelp,
    csvInputRef,
    datInputRef,
    handleCsvChange,
    handleDatChange,
    handleExportCsv,
    triggerDatUpload,
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
    nonEmptyCount,
  } = useWorkbookController();

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

      <AppHeader onHelp={openHelp} onLoadDat={triggerDatUpload} onExportDat={handleExportDat} />

      <main className="workspace">
        <WorkbookToolbar
          activeTab={activeTab}
          onTabChange={setActiveTab}
          visibleActions={visibleActions}
          onExportCsv={handleExportCsv}
        />

        <FormulaBar
          activeCellId={activeCell?.cellId ?? 'A1'}
          formulaDraft={formulaDraft}
          inputRef={formulaInputRef}
          onSubmit={handleFormulaSubmit}
          onFocus={handleFormulaFocus}
          onChange={handleFormulaChange}
        />

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
            nonEmptyCount={nonEmptyCount}
          />
        </div>
      </main>

      <HelpModal open={helpOpen} onClose={closeHelp} />
    </div>
  );
}
