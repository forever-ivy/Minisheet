import { useCallback, useLayoutEffect, useMemo, useRef, useState } from 'react';
import {
  DataEditor,
  GridCellKind,
  getDefaultTheme,
  type EditableGridCell,
  type GridColumn,
  type Item,
} from '@glideapps/glide-data-grid';

import {
  ActiveCell,
  GridSelection,
  WorkbookSnapshot,
  buildGlideCell,
  columnIndexToName,
  glideSelectionToGridSelection,
  gridCoordsToCellId,
  gridSelectionToGlideSelection,
} from './adapter';
import './green-glide.css';

type GreenGlideGridProps = {
  snapshot: WorkbookSnapshot;
  activeCell: ActiveCell | null;
  selection: GridSelection;
  onGridPointerDown?: () => void;
  onSelectionChange: (selection: GridSelection, activeCell: ActiveCell | null) => void;
  onSelectionFinish?: () => void;
  onCellCommit: (cellId: string, raw: string) => Promise<void> | void;
  onDeleteSelection?: (selection: GridSelection) => Promise<void> | void;
  onSelectAll?: () => Promise<void> | void;
};

const gridTheme = {
  ...getDefaultTheme(),
  accentColor: '#059669',
  accentFg: '#ffffff',
  accentLight: '#DCF7E3',
  textDark: '#064E3B',
  textMedium: '#374151',
  textLight: '#9CA3AF',
  textBubble: '#064E3B',
  bgIconHeader: '#F0FBF4',
  fgIconHeader: '#10B981',
  textHeader: '#374151',
  textHeaderSelected: '#059669',
  bgCell: '#FFFFFF',
  bgCellMedium: '#F8FAFC',
  bgHeader: '#F0FBF4',
  bgHeaderHasFocus: '#DCF7E3',
  bgHeaderHovered: '#E7F8EC',
  bgBubble: '#DCF7E3',
  bgBubbleSelected: '#C9F0D4',
  bgSearchResult: '#ECFDF5',
  borderColor: '#E5E7EB',
  horizontalBorderColor: '#E5E7EB',
  headerBottomBorderColor: '#D1D5DB',
  drilldownBorder: '#D1D5DB',
  linkColor: '#059669',
  cellHorizontalPadding: 8,
  cellVerticalPadding: 6,
  headerFontStyle: '600 11px Inter, PingFang SC, system-ui, sans-serif',
  baseFontStyle: '500 13px Inter, PingFang SC, system-ui, sans-serif',
  markerFontStyle: '500 11px Inter, PingFang SC, system-ui, sans-serif',
  fontFamily: 'Inter, PingFang SC, system-ui, sans-serif',
  editorFontSize: '13px',
  lineHeight: 1.3,
  roundingRadius: 10,
};

function editableCellToRaw(value: EditableGridCell): string {
  switch (value.kind) {
    case GridCellKind.Number: {
      if (typeof value.data === 'number' && Number.isFinite(value.data)) {
        return String(value.data);
      }

      if ('displayData' in value && typeof value.displayData === 'string') {
        return value.displayData.replace(/,/g, '').trim();
      }

      return '';
    }
    case GridCellKind.Text:
    case GridCellKind.Uri:
    case GridCellKind.Markdown:
      return value.data;
    case GridCellKind.Boolean:
      return value.data == null ? '' : String(value.data);
    default:
      return 'data' in value && typeof value.data === 'string' ? value.data : '';
  }
}

export function GreenGlideGrid({
  snapshot,
  activeCell,
  selection,
  onGridPointerDown,
  onSelectionChange,
  onSelectionFinish,
  onCellCommit,
  onDeleteSelection,
  onSelectAll,
}: GreenGlideGridProps) {
  const containerRef = useRef<HTMLDivElement | null>(null);
  const [containerSize, setContainerSize] = useState(() => ({ width: 900, height: 560 }));

  useLayoutEffect(() => {
    const element = containerRef.current;
    if (!element) {
      return;
    }

    const measure = () => {
      const rect = element.getBoundingClientRect();
      const width = Math.max(1, Math.floor(rect.width));
      const height = Math.max(1, Math.floor(rect.height));
      setContainerSize((previous) => (previous.width === width && previous.height === height ? previous : { width, height }));
    };

    measure();

    if (typeof ResizeObserver === 'undefined') {
      window.addEventListener('resize', measure);
      return () => window.removeEventListener('resize', measure);
    }

    const observer = new ResizeObserver(measure);
    observer.observe(element);
    return () => observer.disconnect();
  }, []);

  const columns = useMemo<readonly GridColumn[]>(
    () =>
      Array.from({ length: snapshot.maxCols }, (_, index) => ({
        id: columnIndexToName(index),
        title: columnIndexToName(index),
        width: index === 0 ? 170 : 100,
      })),
    [snapshot.maxCols],
  );

  const glideSelection = useMemo(() => gridSelectionToGlideSelection(selection), [selection]);

  const getCellContent = useCallback(
    ([col, row]: Item) => buildGlideCell(snapshot.cells[gridCoordsToCellId(row, col)], row, col),
    [snapshot.cells],
  );

  const handleGridSelectionChange = useCallback(
    (nextSelection: typeof glideSelection) => {
      const nextGridSelection = glideSelectionToGridSelection(nextSelection);
      const nextActiveCell = nextSelection.current
        ? {
            row: nextSelection.current.cell[1],
            col: nextSelection.current.cell[0],
            cellId: gridCoordsToCellId(nextSelection.current.cell[1], nextSelection.current.cell[0]),
          }
        : null;
      onSelectionChange(nextGridSelection, nextActiveCell);
    },
    [onSelectionChange],
  );

  const handleCellEdited = useCallback(
    (cell: Item, newValue: EditableGridCell) => {
      const [col, row] = cell;
      void onCellCommit(gridCoordsToCellId(row, col), editableCellToRaw(newValue));
    },
    [onCellCommit],
  );

  const handleDelete = useCallback(
    (nextSelection: typeof glideSelection) => {
      const nextGridSelection = glideSelectionToGridSelection(nextSelection);
      void onDeleteSelection?.(nextGridSelection);
      return true;
    },
    [onDeleteSelection],
  );

  const handleGridKeyDown = useCallback(
    (event: {
      key: string;
      ctrlKey: boolean;
      metaKey: boolean;
      cancel: () => void;
      stopPropagation: () => void;
      preventDefault: () => void;
    }) => {
      if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === 'a') {
        event.cancel();
        event.stopPropagation();
        event.preventDefault();
        void onSelectAll?.();
        return;
      }

      if (event.key === 'Delete' || event.key === 'Backspace') {
        event.cancel();
        event.stopPropagation();
        event.preventDefault();
        void onDeleteSelection?.(selection);
      }
    },
    [onDeleteSelection, onSelectAll, selection],
  );

  const handlePointerDownCapture = useCallback(() => {
    onGridPointerDown?.();
  }, [onGridPointerDown]);

  return (
    <div
      ref={containerRef}
      className="green-glide-shell"
      data-testid="green-glide-shell"
      onPointerDownCapture={handlePointerDownCapture}
      onPointerUpCapture={onSelectionFinish}
    >
      <DataEditor
        className="green-glide-editor"
        width={containerSize.width}
        height={containerSize.height}
        rows={snapshot.maxRows}
        columns={columns}
        rowMarkers={{ kind: 'number', startIndex: 1, width: 50 }}
        rowHeight={32}
        headerHeight={32}
        getCellContent={getCellContent}
        getCellsForSelection={true}
        gridSelection={glideSelection}
        onGridSelectionChange={handleGridSelectionChange}
        onCellEdited={handleCellEdited}
        onDelete={handleDelete}
        onKeyDown={handleGridKeyDown}
        editOnType
        cellActivationBehavior="second-click"
        rangeSelect="rect"
        columnSelect="none"
        rowSelect="none"
        smoothScrollX
        smoothScrollY
        freezeColumns={0}
        theme={gridTheme}
      />
    </div>
  );
}
