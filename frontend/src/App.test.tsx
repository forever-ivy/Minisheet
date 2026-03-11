import { act, fireEvent, render, screen, waitFor } from '@testing-library/react';

import App from './App';
import { CellDto } from './spreadsheet/adapter';

const createObjectURLMock = jest.fn(() => 'blob:mock');
const revokeObjectURLMock = jest.fn();
const anchorClickMock = jest.fn();
const browserDraftKey = 'minisheet.browserDraft.v1';

jest.mock(
  './spreadsheet/GreenGlideGrid',
  () => {
    const React = require('react');

    return {
      GreenGlideGrid: ({
        onGridPointerDown,
        onSelectionChange,
        onSelectionFinish,
        onCellCommit,
        onDeleteSelection,
        onSelectAll,
      }: {
        onGridPointerDown?: () => void;
        onSelectionChange: (selection: unknown, activeCell: unknown) => void;
        onSelectionFinish?: () => void;
        onCellCommit?: (cellId: string, raw: string) => Promise<void> | void;
        onDeleteSelection?: (selection: unknown) => Promise<void> | void;
        onSelectAll?: () => Promise<void> | void;
      }) => (
        <div data-testid="green-glide-grid">
          <button
            type="button"
            onClick={() => {
              onGridPointerDown?.();
              onSelectionChange(
                { startRow: 1, startCol: 1, endRow: 2, endCol: 2 },
                { row: 1, col: 1, cellId: 'B2' },
              );
              onSelectionFinish?.();
            }}
          >
            模拟选择 B2:C3
          </button>
          <button
            type="button"
            onClick={() => {
              onSelectionChange(
                { startRow: 1, startCol: 1, endRow: 1, endCol: 1 },
                { row: 1, col: 1, cellId: 'B2' },
              );
              onSelectionFinish?.();
            }}
          >
            模拟被动选中 B2
          </button>
          <button
            type="button"
            onClick={() => {
              onGridPointerDown?.();
              onSelectionChange(
                { startRow: 3, startCol: 3, endRow: 3, endCol: 3 },
                { row: 3, col: 3, cellId: 'D4' },
              );
              onSelectionFinish?.();
            }}
          >
            模拟激活 D4
          </button>
          <button
            type="button"
            onClick={() => {
              onSelectionChange(
                { startRow: 3, startCol: 3, endRow: 3, endCol: 3 },
                { row: 3, col: 3, cellId: 'D4' },
              );
              void onCellCommit?.('B2', '999');
              onSelectionFinish?.();
            }}
          >
            模拟切到 D4 同时提交 B2 编辑
          </button>
          <button
            type="button"
            onClick={() => {
              void onSelectAll?.();
            }}
          >
            模拟全选
          </button>
          <button
            type="button"
            onClick={() => {
              void onDeleteSelection?.(undefined);
            }}
          >
            模拟删除当前选区
          </button>
        </div>
      ),
    };
  },
  { virtual: true },
);

const baseSnapshot = {
  maxRows: 32767,
  maxCols: 256,
  computeMs: 5.25,
  cells: {
    B2: { id: 'B2', raw: '12500', display: '12500', type: 'integer', error: '' },
    C2: { id: 'C2', raw: '13000', display: '13000', type: 'integer', error: '' },
    B3: { id: 'B3', raw: '8200', display: '8200', type: 'integer', error: '' },
    C3: { id: 'C3', raw: '9500', display: '9500', type: 'integer', error: '' },
    D4: { id: 'D4', raw: '=SUM(B2:C3)', display: '43200', type: 'formula', error: '' },
  },
};

const restoredSnapshot = {
  maxRows: 32767,
  maxCols: 256,
  computeMs: 1.5,
  cells: {
    A1: { id: 'A1', raw: '77', display: '77', type: 'integer', error: '' },
    B1: { id: 'B1', raw: '=A1+1', display: '78', type: 'formula', error: '' },
  },
};

function mockJsonResponse(body: unknown): Promise<Response> {
  return Promise.resolve({
    ok: true,
    json: async () => body,
    text: async () => JSON.stringify(body),
  } as Response);
}

function mockBlobResponse(body: Blob): Promise<Response> {
  return Promise.resolve({
    ok: true,
    blob: async () => body,
    text: async () => '',
  } as Response);
}

beforeEach(() => {
  Object.defineProperty(window.URL, 'createObjectURL', {
    writable: true,
    value: createObjectURLMock,
  });
  Object.defineProperty(window.URL, 'revokeObjectURL', {
    writable: true,
    value: revokeObjectURLMock,
  });
  HTMLAnchorElement.prototype.click = anchorClickMock;
  createObjectURLMock.mockClear();
  revokeObjectURLMock.mockClear();
  anchorClickMock.mockClear();
  window.sessionStorage.clear();

  global.fetch = jest.fn((input: RequestInfo | URL, init?: RequestInit) => {
    if (typeof input === 'string' && input.endsWith('/api/restore-browser-draft') && init?.method === 'POST') {
      return mockJsonResponse(restoredSnapshot);
    }

    if (typeof input === 'string' && input.endsWith('/api/save-dat') && init?.method === 'POST') {
      return mockBlobResponse(new Blob(['mock-dat'], { type: 'application/octet-stream' }));
    }

    if (typeof input === 'string' && input.endsWith('/api/cell') && init?.method === 'POST') {
      const body = JSON.parse((init.body as string) || '{}');
      const nextCells: Record<string, CellDto> = { ...baseSnapshot.cells };

      if (body.raw === '') {
        delete nextCells[body.cellId];
      } else {
        nextCells[body.cellId] = {
          id: body.cellId,
          raw: body.raw,
          display: body.raw,
          type: body.raw?.startsWith('=') ? 'formula' : 'string',
          error: '',
        };
      }

      return mockJsonResponse({
        ...baseSnapshot,
        cells: nextCells,
      });
    }

    return mockJsonResponse(baseSnapshot);
  }) as jest.Mock;
});

afterEach(() => {
  jest.resetAllMocks();
});

function getFormulaInput() {
  return screen.getByRole('textbox', { name: '公式栏' });
}

async function waitForWorkbookReady() {
  await screen.findByText('5 个非空单元格');
}

test('renders the green workspace shell without the insight panel', async () => {
  render(<App />);

  await waitForWorkbookReady();

  expect(screen.getByText('MiniSheet')).toBeInTheDocument();
  expect(screen.getByRole('button', { name: '主页' })).toBeInTheDocument();
  expect(screen.getByRole('button', { name: '插入' })).toBeInTheDocument();
  expect(screen.queryByRole('button', { name: '数据' })).not.toBeInTheDocument();
  expect(screen.queryByPlaceholderText('搜索命令...')).not.toBeInTheDocument();
  expect(screen.getByRole('button', { name: '帮助' })).toBeInTheDocument();
  expect(screen.getByRole('button', { name: '载入 DAT' })).toBeInTheDocument();
  expect(screen.getByRole('button', { name: '导出 DAT' })).toBeInTheDocument();
  expect(screen.getAllByRole('button', { name: '导出 DAT' })).toHaveLength(1);
  expect(screen.getByRole('button', { name: '导出 CSV' })).toBeInTheDocument();
  expect(screen.getByRole('button', { name: '保存' })).toBeInTheDocument();
  expect(screen.queryByRole('button', { name: '刷新' })).not.toBeInTheDocument();
  expect(screen.queryByText('已自动保存')).not.toBeInTheDocument();
  expect(screen.queryByRole('button', { name: '刷新快照' })).not.toBeInTheDocument();
  expect(screen.queryByRole('button', { name: '重新同步' })).not.toBeInTheDocument();
  expect(screen.queryByRole('button', { name: '切换洞察面板' })).not.toBeInTheDocument();
  expect(screen.queryByLabelText('数据洞察')).not.toBeInTheDocument();
  expect(screen.getByTestId('green-glide-grid')).toBeInTheDocument();
});

test('switches action groups when the toolbar tab changes', async () => {
  render(<App />);
  await waitForWorkbookReady();

  expect(screen.getByRole('button', { name: '导入 CSV' })).toBeInTheDocument();
  expect(screen.queryByRole('button', { name: '载入 DAT' })).not.toHaveClass('pill-btn');
  expect(screen.getByRole('button', { name: '保存' })).toBeInTheDocument();

  fireEvent.click(screen.getByRole('button', { name: '插入' }));

  expect(screen.getByRole('button', { name: '求和' })).toBeInTheDocument();
  expect(screen.getByRole('button', { name: '平均值' })).toBeInTheDocument();
  expect(screen.queryByRole('button', { name: '导入 CSV' })).not.toBeInTheDocument();
  expect(screen.queryByRole('button', { name: '数据' })).not.toBeInTheDocument();
});

test('saves the current workbook into sessionStorage when 保存 is clicked', async () => {
  render(<App />);
  await waitForWorkbookReady();

  fireEvent.click(screen.getByRole('button', { name: '保存' }));

  const saved = JSON.parse(window.sessionStorage.getItem(browserDraftKey) || '{}');
  expect(saved.workbookTitle).toBe('未命名');
  expect(saved.cells).toEqual({
    B2: '12500',
    C2: '13000',
    B3: '8200',
    C3: '9500',
    D4: '=SUM(B2:C3)',
  });
  expect(typeof saved.savedAt).toBe('string');
});

test('allows editing the workbook title and persists it when saving', async () => {
  render(<App />);
  await waitForWorkbookReady();

  fireEvent.change(screen.getByRole('textbox', { name: '工作簿名称' }), {
    target: { value: '期末成绩' },
  });
  fireEvent.click(screen.getByRole('button', { name: '保存' }));

  expect(screen.getByDisplayValue('期末成绩')).toBeInTheDocument();

  const saved = JSON.parse(window.sessionStorage.getItem(browserDraftKey) || '{}');
  expect(saved.workbookTitle).toBe('期末成绩');
});

test('restores a saved browser draft before loading the default snapshot', async () => {
  window.sessionStorage.setItem(
    browserDraftKey,
    JSON.stringify({
      workbookTitle: '草稿工作簿',
      cells: {
        A1: '77',
        B1: '=A1+1',
      },
      savedAt: '2026-03-11T12:00:00.000Z',
    }),
  );

  render(<App />);

  await screen.findByText('2 个非空单元格');

  expect((global.fetch as jest.Mock).mock.calls.some(([url, init]) =>
    typeof url === 'string' &&
    url.includes('/api/restore-browser-draft') &&
    (init as RequestInit | undefined)?.method === 'POST',
  )).toBe(true);
  expect((global.fetch as jest.Mock).mock.calls.some(([url]) =>
    typeof url === 'string' && url.includes('/api/snapshot'),
  )).toBe(false);
  expect(screen.getByDisplayValue('77')).toBeInTheDocument();
  expect(screen.getByDisplayValue('草稿工作簿')).toBeInTheDocument();
});

test('clears invalid browser drafts and falls back to snapshot loading', async () => {
  window.sessionStorage.setItem(browserDraftKey, '{not-valid-json');

  render(<App />);
  await waitForWorkbookReady();

  expect(window.sessionStorage.getItem(browserDraftKey)).toBeNull();
  expect((global.fetch as jest.Mock).mock.calls.some(([url]) =>
    typeof url === 'string' && url.includes('/api/snapshot'),
  )).toBe(true);
});

test('falls back to snapshot loading when browser draft restore fails', async () => {
  window.sessionStorage.setItem(
    browserDraftKey,
    JSON.stringify({
      workbookTitle: '失败草稿',
      cells: {
        A1: '1',
      },
      savedAt: '2026-03-11T12:00:00.000Z',
    }),
  );

  (global.fetch as jest.Mock).mockImplementation((input: RequestInfo | URL, init?: RequestInit) => {
    if (typeof input === 'string' && input.endsWith('/api/restore-browser-draft') && init?.method === 'POST') {
      return Promise.resolve({
        ok: false,
        text: async () => 'restore failed',
      } as Response);
    }

    if (typeof input === 'string' && input.endsWith('/api/save-dat') && init?.method === 'POST') {
      return mockBlobResponse(new Blob(['mock-dat'], { type: 'application/octet-stream' }));
    }

    if (typeof input === 'string' && input.endsWith('/api/cell') && init?.method === 'POST') {
      const body = JSON.parse((init.body as string) || '{}');
      const nextCells: Record<string, CellDto> = { ...baseSnapshot.cells };

      if (body.raw === '') {
        delete nextCells[body.cellId];
      } else {
        nextCells[body.cellId] = {
          id: body.cellId,
          raw: body.raw,
          display: body.raw,
          type: body.raw?.startsWith('=') ? 'formula' : 'string',
          error: '',
        };
      }

      return mockJsonResponse({
        ...baseSnapshot,
        cells: nextCells,
      });
    }

    return mockJsonResponse(baseSnapshot);
  });

  render(<App />);
  await waitForWorkbookReady();

  expect(window.sessionStorage.getItem(browserDraftKey)).toBeNull();
  expect((global.fetch as jest.Mock).mock.calls.some(([url]) =>
    typeof url === 'string' && url.includes('/api/snapshot'),
  )).toBe(true);
});

test('fills uppercase formulas from the current glide selection without auto-submitting', async () => {
  render(<App />);
  await waitForWorkbookReady();

  fireEvent.click(screen.getByRole('button', { name: '插入' }));
  fireEvent.click(screen.getByRole('button', { name: '模拟激活 D4' }));
  fireEvent.click(screen.getByRole('button', { name: '求和' }));

  await act(async () => {
    fireEvent.click(screen.getByRole('button', { name: '模拟选择 B2:C3' }));
  });

  await waitFor(() =>
    expect(global.fetch).toHaveBeenCalledWith(
      expect.stringContaining('/api/cell'),
      expect.objectContaining({
        method: 'POST',
        body: JSON.stringify({ cellId: 'D4', raw: '=SUM(B2:C3)' }),
      }),
    ),
  );
});

test('does not rewrite the formula draft on passive grid selection updates', async () => {
  render(<App />);
  await waitForWorkbookReady();

  const formulaInput = getFormulaInput();
  fireEvent.focus(formulaInput);
  fireEvent.change(formulaInput, { target: { value: '=SUM(B2:C3)' } });
  fireEvent.blur(formulaInput);
  fireEvent.click(screen.getByRole('button', { name: '模拟被动选中 B2' }));

  await waitFor(() => expect(formulaInput).toHaveValue('=SUM(B2:C3)'));
});

test('cancels a pending toolbar formula action when Escape is pressed', async () => {
  render(<App />);
  await waitForWorkbookReady();

  fireEvent.click(screen.getByRole('button', { name: '插入' }));
  fireEvent.click(screen.getByRole('button', { name: '模拟激活 D4' }));
  fireEvent.click(screen.getByRole('button', { name: '求和' }));

  fireEvent.keyDown(document, { key: 'Escape' });
  fireEvent.click(screen.getByRole('button', { name: '模拟选择 B2:C3' }));

  await waitFor(() => expect(global.fetch).toHaveBeenCalledTimes(1));
});

test('does not render the insight panel even after grid selection changes', async () => {
  render(<App />);
  await waitForWorkbookReady();

  fireEvent.click(screen.getByRole('button', { name: '模拟选择 B2:C3' }));

  expect(screen.queryByLabelText('数据洞察')).not.toBeInTheDocument();
});

test('submits the formula bar to the current target cell', async () => {
  render(<App />);
  await waitForWorkbookReady();

  fireEvent.click(screen.getByRole('button', { name: '模拟激活 D4' }));

  await act(async () => {
    fireEvent.change(getFormulaInput(), { target: { value: '=ABS(B2)' } });
    fireEvent.click(screen.getByRole('button', { name: '提交公式' }));
  });

  await waitFor(() =>
    expect(global.fetch).toHaveBeenCalledWith(
      expect.stringContaining('/api/cell'),
      expect.objectContaining({
        method: 'POST',
        body: JSON.stringify({ cellId: 'D4', raw: '=ABS(B2)' }),
      }),
    ),
  );
});

test('submits plain numeric input from the formula bar', async () => {
  render(<App />);
  await waitForWorkbookReady();

  await act(async () => {
    fireEvent.change(getFormulaInput(), { target: { value: '123' } });
    fireEvent.click(screen.getByRole('button', { name: '提交公式' }));
  });

  await waitFor(() =>
    expect(global.fetch).toHaveBeenCalledWith(
      expect.stringContaining('/api/cell'),
      expect.objectContaining({
        method: 'POST',
        body: JSON.stringify({ cellId: 'A1', raw: '123' }),
      }),
    ),
  );
});

test('retargets plain-value editing when selection changes to a new cell', async () => {
  render(<App />);
  await waitForWorkbookReady();

  fireEvent.click(screen.getByRole('button', { name: '模拟被动选中 B2' }));

  await act(async () => {
    fireEvent.change(getFormulaInput(), { target: { value: '123' } });
  });

  fireEvent.click(screen.getByRole('button', { name: '模拟激活 D4' }));

  await act(async () => {
    fireEvent.change(getFormulaInput(), { target: { value: '456' } });
    fireEvent.click(screen.getByRole('button', { name: '提交公式' }));
  });

  await waitFor(() =>
    expect(global.fetch).toHaveBeenCalledWith(
      expect.stringContaining('/api/cell'),
      expect.objectContaining({
        method: 'POST',
        body: JSON.stringify({ cellId: 'D4', raw: '456' }),
      }),
    ),
  );
});

test('moves the active cell with keyboard navigation keys', async () => {
  render(<App />);
  await waitForWorkbookReady();

  expect(document.querySelector('.cell-ref')?.textContent).toBe('A1');

  fireEvent.keyDown(document, { key: 'ArrowRight', code: 'Numpad6' });
  expect(document.querySelector('.cell-ref')?.textContent).toBe('B1');

  fireEvent.keyDown(document, { key: 'ArrowDown', code: 'Numpad2' });
  expect(document.querySelector('.cell-ref')?.textContent).toBe('B2');
  expect(getFormulaInput()).toHaveValue('12500');
});

test('moves the active cell with numpad digits when NumLock is on', async () => {
  render(<App />);
  await waitForWorkbookReady();

  expect(document.querySelector('.cell-ref')?.textContent).toBe('A1');

  fireEvent.keyDown(document, { key: '6', code: 'Numpad6' });
  expect(document.querySelector('.cell-ref')?.textContent).toBe('B1');

  fireEvent.keyDown(document, { key: '2', code: 'Numpad2' });
  expect(document.querySelector('.cell-ref')?.textContent).toBe('B2');
});

test('does not snap back to the previous cell when a prior edit resolves after selection change', async () => {
  render(<App />);
  await waitForWorkbookReady();

  await act(async () => {
    fireEvent.click(screen.getByRole('button', { name: '模拟切到 D4 同时提交 B2 编辑' }));
  });

  await waitFor(() =>
    expect(global.fetch).toHaveBeenCalledWith(
      expect.stringContaining('/api/cell'),
      expect.objectContaining({
        method: 'POST',
        body: JSON.stringify({ cellId: 'B2', raw: '999' }),
      }),
    ),
  );

  expect(document.querySelector('.cell-ref')?.textContent).toBe('D4');
  expect(getFormulaInput()).toHaveValue('=SUM(B2:C3)');
});

test('selects all cells and clears non-empty values with delete', async () => {
  render(<App />);
  await waitForWorkbookReady();

  fireEvent.click(screen.getByRole('button', { name: '模拟全选' }));
  fireEvent.click(screen.getByRole('button', { name: '模拟删除当前选区' }));

  await waitFor(() => {
    const cellCalls = (global.fetch as jest.Mock).mock.calls.filter(([url]) =>
      typeof url === 'string' && url.includes('/api/cell'),
    );

    expect(cellCalls).toHaveLength(5);
  });

  const committedBodies = (global.fetch as jest.Mock).mock.calls
    .filter(([url]) => typeof url === 'string' && url.includes('/api/cell'))
    .map(([, init]) => JSON.parse((init as RequestInit).body as string));

  expect(committedBodies).toEqual([
    { cellId: 'B2', raw: '' },
    { cellId: 'C2', raw: '' },
    { cellId: 'B3', raw: '' },
    { cellId: 'C3', raw: '' },
    { cellId: 'D4', raw: '' },
  ]);
});

test('clears a mouse-selected range with delete', async () => {
  render(<App />);
  await waitForWorkbookReady();

  fireEvent.click(screen.getByRole('button', { name: '模拟选择 B2:C3' }));
  fireEvent.click(screen.getByRole('button', { name: '模拟删除当前选区' }));

  await waitFor(() => {
    const cellCalls = (global.fetch as jest.Mock).mock.calls.filter(([url]) =>
      typeof url === 'string' && url.includes('/api/cell'),
    );

    expect(cellCalls).toHaveLength(4);
  });

  const committedBodies = (global.fetch as jest.Mock).mock.calls
    .filter(([url]) => typeof url === 'string' && url.includes('/api/cell'))
    .map(([, init]) => JSON.parse((init as RequestInit).body as string));

  expect(committedBodies).toEqual([
    { cellId: 'B2', raw: '' },
    { cellId: 'C2', raw: '' },
    { cellId: 'B3', raw: '' },
    { cellId: 'C3', raw: '' },
  ]);
});

test('exports the current workbook as csv without calling save-dat', async () => {
  render(<App />);
  await waitForWorkbookReady();

  await act(async () => {
    fireEvent.click(screen.getByRole('button', { name: '导出 CSV' }));
  });

  expect(createObjectURLMock).toHaveBeenCalledTimes(1);
  expect(anchorClickMock).toHaveBeenCalledTimes(1);
  expect(revokeObjectURLMock).toHaveBeenCalledTimes(1);
  expect((global.fetch as jest.Mock).mock.calls.some(([url]) =>
    typeof url === 'string' && url.includes('/api/save-dat'),
  )).toBe(false);
});

test('exports dat from the header badge', async () => {
  render(<App />);
  await waitForWorkbookReady();

  await act(async () => {
    fireEvent.click(screen.getByRole('button', { name: '导出 DAT' }));
  });

  expect((global.fetch as jest.Mock).mock.calls.some(([url, init]) =>
    typeof url === 'string' && url.includes('/api/save-dat') && (init as RequestInit | undefined)?.method === 'POST',
  )).toBe(true);
  expect(createObjectURLMock).toHaveBeenCalledTimes(1);
  expect(anchorClickMock).toHaveBeenCalledTimes(1);
  expect(revokeObjectURLMock).toHaveBeenCalledTimes(1);
});
