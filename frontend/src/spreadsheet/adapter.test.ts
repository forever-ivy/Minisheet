import {
  applySelectionToFormula,
  buildFunctionFormula,
  cellIdToGridCoords,
  getCellDtoAt,
  getCellTypeLabel,
  glideSelectionToGridSelection,
  gridCoordsToCellId,
  gridSelectionToGlideSelection,
  gridSelectionToRangeRef,
} from './adapter';

const snapshot = {
  maxRows: 32767,
  maxCols: 256,
  computeMs: 0,
  cells: {
    A1: { id: 'A1', raw: '42', display: '42', type: 'integer', error: '' },
    C3: { id: 'C3', raw: '=A1*2', display: '84', type: 'formula', error: '' },
  },
};

test('converts between grid coordinates and spreadsheet cell ids', () => {
  expect(gridCoordsToCellId(0, 0)).toBe('A1');
  expect(gridCoordsToCellId(9, 26)).toBe('AA10');
  expect(cellIdToGridCoords('C3')).toEqual({ row: 2, col: 2, cellId: 'C3' });
  expect(cellIdToGridCoords('AA10')).toEqual({ row: 9, col: 26, cellId: 'AA10' });
});

test('formats grid selections and formulas in uppercase Excel style', () => {
  const range = { startRow: 1, startCol: 1, endRow: 2, endCol: 2 };

  expect(gridSelectionToRangeRef(range)).toBe('B2:C3');
  expect(applySelectionToFormula('=SUM(', range)).toBe('=SUM(B2:C3)');
  expect(applySelectionToFormula('=SUM(A1)', range)).toBe('=SUM(B2:C3)');
  expect(applySelectionToFormula('=SQRT(A1)', range)).toBe('=SQRT(B2)');
  expect(applySelectionToFormula('=A1+', range)).toBe('=A1+B2');
  expect(applySelectionToFormula('=A1+B1', range)).toBe('=A1+B2');
  expect(buildFunctionFormula('sum', 'A1', range)).toBe('=SUM(B2:C3)');
  expect(buildFunctionFormula('avg', 'C4', null)).toBe('=AVG(C4)');
  expect(buildFunctionFormula('sqrt', 'D5', range)).toBe('=SQRT(D5)');
  expect(buildFunctionFormula('abs', 'E6', null)).toBe('=ABS(E6)');
});

test('translates between glide selections and spreadsheet ranges', () => {
  const gridSelection = { startRow: 1, startCol: 1, endRow: 2, endCol: 2 };

  expect(gridSelectionToGlideSelection(gridSelection)).toMatchObject({
    current: {
      cell: [1, 1],
      range: { x: 1, y: 1, width: 2, height: 2 },
    },
  });

  expect(
    glideSelectionToGridSelection({
      current: {
        cell: [1, 1],
        range: { x: 1, y: 1, width: 2, height: 2 },
        rangeStack: [],
      },
    } as any),
  ).toEqual(gridSelection);
});

test('retrieves workbook cells on demand without building a full raw matrix', () => {
  expect(getCellDtoAt(snapshot, 0, 0)?.raw).toBe('42');
  expect(getCellDtoAt(snapshot, 2, 2)?.display).toBe('84');
  expect(getCellDtoAt(snapshot, 1, 1)).toBeUndefined();
});

test('maps backend cell types to chinese labels', () => {
  expect(getCellTypeLabel('integer')).toBe('整数');
  expect(getCellTypeLabel('float')).toBe('浮点数');
  expect(getCellTypeLabel('string')).toBe('字符串');
  expect(getCellTypeLabel('formula')).toBe('公式');
  expect(getCellTypeLabel('error')).toBe('错误');
  expect(getCellTypeLabel('empty')).toBe('空白');
});
