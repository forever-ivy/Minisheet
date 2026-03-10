import { act, fireEvent, render, screen } from '@testing-library/react';

import { GreenGlideGrid } from './GreenGlideGrid';

const captured = {
  props: null as null | Record<string, unknown>,
};

jest.mock(
  '@glideapps/glide-data-grid',
  () => ({
    CompactSelection: {
      empty: () => ({ items: [] }),
    },
    GridCellKind: {
      Text: 'text',
      Number: 'number',
      Uri: 'uri',
      Markdown: 'markdown',
      Boolean: 'boolean',
    },
    getDefaultTheme: () => ({
      accentColor: '#059669',
      accentFg: '#fff',
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
      drilldownBorder: '#D1D5DB',
      linkColor: '#059669',
      cellHorizontalPadding: 8,
      cellVerticalPadding: 6,
      headerFontStyle: '600 11px Inter',
      headerIconSize: 16,
      baseFontStyle: '500 13px Inter',
      markerFontStyle: '500 11px Inter',
      fontFamily: 'Inter',
      editorFontSize: '13px',
      lineHeight: 1.3,
    }),
    DataEditor: (props: Record<string, unknown>) => {
      captured.props = props;
      return <div data-testid="data-editor" />;
    },
  }),
  { virtual: true },
);

const snapshot = {
  maxRows: 32767,
  maxCols: 256,
  computeMs: 0,
  cells: {
    A1: { id: 'A1', raw: '42', display: '42', type: 'integer', error: '' },
    B2: { id: 'B2', raw: '=A1', display: '42', type: 'formula', error: '' },
  },
};

test('passes Glide grid config and bridges selection/edit hooks', async () => {
  const onSelectionChange = jest.fn();
  const onSelectionFinish = jest.fn();
  const onCellCommit = jest.fn();
  const onGridPointerDown = jest.fn();

  render(
    <GreenGlideGrid
      snapshot={snapshot}
      activeCell={{ row: 0, col: 0, cellId: 'A1' }}
      selection={{ startRow: 0, startCol: 0, endRow: 0, endCol: 0 }}
      onGridPointerDown={onGridPointerDown}
      onSelectionChange={onSelectionChange}
      onSelectionFinish={onSelectionFinish}
      onCellCommit={onCellCommit}
    />,
  );

  expect(screen.getByTestId('data-editor')).toBeInTheDocument();
  expect(captured.props?.rows).toBe(32767);
  expect(captured.props?.rangeSelect).toBe('rect');
  expect(captured.props?.rowSelect).toBe('none');
  expect(captured.props?.columnSelect).toBe('none');
  expect(captured.props?.getCellsForSelection).toBe(true);
  expect(typeof captured.props?.height).toBe('number');

  fireEvent.pointerDown(screen.getByTestId('green-glide-shell'));
  fireEvent.pointerUp(screen.getByTestId('green-glide-shell'));

  expect(onGridPointerDown).toHaveBeenCalledTimes(1);
  expect(onSelectionFinish).toHaveBeenCalledTimes(1);

  await act(async () => {
    (captured.props?.onGridSelectionChange as (selection: unknown) => void)({
      current: {
        cell: [1, 1],
        range: { x: 1, y: 1, width: 2, height: 2 },
        rangeStack: [],
      },
      columns: { items: [] },
      rows: { items: [] },
    });
  });

  expect(onSelectionChange).toHaveBeenCalledWith(
    { startRow: 1, startCol: 1, endRow: 2, endCol: 2 },
    { row: 1, col: 1, cellId: 'B2' },
  );

  await act(async () => {
    await (captured.props?.onCellEdited as (cell: [number, number], value: { kind: string; data: string }) => void)(
      [1, 1],
      { kind: 'text', data: '=A1+1' },
    );
  });

  expect(onCellCommit).toHaveBeenCalledWith('B2', '=A1+1');
});
