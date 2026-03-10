import { act, fireEvent, render, screen, waitFor, within } from '@testing-library/react';

import App from './App';

jest.mock(
  './spreadsheet/GreenGlideGrid',
  () => {
    const React = require('react');

    return {
      GreenGlideGrid: ({
        onGridPointerDown,
        onSelectionChange,
        onSelectionFinish,
      }: {
        onGridPointerDown?: () => void;
        onSelectionChange: (selection: unknown, activeCell: unknown) => void;
        onSelectionFinish?: () => void;
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

function mockJsonResponse(body: unknown): Promise<Response> {
  return Promise.resolve({
    ok: true,
    json: async () => body,
    text: async () => JSON.stringify(body),
  } as Response);
}

beforeEach(() => {
  global.fetch = jest.fn((input: RequestInfo | URL, init?: RequestInit) => {
    if (typeof input === 'string' && input.endsWith('/api/cell') && init?.method === 'POST') {
      const body = JSON.parse((init.body as string) || '{}');
      return mockJsonResponse({
        ...baseSnapshot,
        cells: {
          ...baseSnapshot.cells,
          [body.cellId]: {
            id: body.cellId,
            raw: body.raw,
            display: body.raw,
            type: body.raw?.startsWith('=') ? 'formula' : 'string',
            error: '',
          },
        },
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

test('renders the green workspace shell with chinese tabs and insight panel', async () => {
  render(<App />);

  await screen.findByText('已自动保存');

  expect(screen.getByText('s.Sheet')).toBeInTheDocument();
  expect(screen.getByRole('button', { name: '主页' })).toBeInTheDocument();
  expect(screen.getByRole('button', { name: '插入' })).toBeInTheDocument();
  expect(screen.getByRole('button', { name: '数据' })).toBeInTheDocument();
  expect(screen.getByPlaceholderText('搜索命令...')).toBeInTheDocument();
  expect(screen.getByRole('button', { name: '导出 DAT' })).toBeInTheDocument();
  expect(screen.getByText('数据洞察')).toBeInTheDocument();
  expect(screen.getByTestId('green-glide-grid')).toBeInTheDocument();
});

test('switches action groups when the toolbar tab changes', async () => {
  render(<App />);
  await screen.findByText('已自动保存');

  expect(screen.getByRole('button', { name: '导入 CSV' })).toBeInTheDocument();

  fireEvent.click(screen.getByRole('button', { name: '插入' }));

  expect(screen.getByRole('button', { name: '求和' })).toBeInTheDocument();
  expect(screen.getByRole('button', { name: '平均值' })).toBeInTheDocument();
  expect(screen.queryByRole('button', { name: '导入 CSV' })).not.toBeInTheDocument();
});

test('fills uppercase formulas from the current glide selection without auto-submitting', async () => {
  render(<App />);
  await screen.findByText('已自动保存');

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
  await screen.findByText('已自动保存');

  const formulaInput = getFormulaInput();
  fireEvent.focus(formulaInput);
  fireEvent.change(formulaInput, { target: { value: '=SUM(B2:C3)' } });
  fireEvent.blur(formulaInput);
  fireEvent.click(screen.getByRole('button', { name: '模拟被动选中 B2' }));

  await waitFor(() => expect(formulaInput).toHaveValue('=SUM(B2:C3)'));
});

test('cancels a pending toolbar formula action when Escape is pressed', async () => {
  render(<App />);
  await screen.findByText('已自动保存');

  fireEvent.click(screen.getByRole('button', { name: '插入' }));
  fireEvent.click(screen.getByRole('button', { name: '模拟激活 D4' }));
  fireEvent.click(screen.getByRole('button', { name: '求和' }));

  fireEvent.keyDown(document, { key: 'Escape' });
  fireEvent.click(screen.getByRole('button', { name: '模拟选择 B2:C3' }));

  await waitFor(() => expect(global.fetch).toHaveBeenCalledTimes(1));
});

test('updates the insight panel from the current selected numeric range', async () => {
  render(<App />);
  await screen.findByText('已自动保存');

  fireEvent.click(screen.getByRole('button', { name: '模拟选择 B2:C3' }));

  const panel = screen.getByLabelText('数据洞察');
  await within(panel).findByText('43,200');
  expect(within(panel).getByText('-24.0%')).toBeInTheDocument();
});

test('renders a zeroed insight panel when the workbook is empty', async () => {
  (global.fetch as jest.Mock).mockImplementation(() =>
    mockJsonResponse({
      maxRows: 32767,
      maxCols: 256,
      computeMs: 0,
      cells: {},
    }),
  );

  render(<App />);

  await screen.findByText('已自动保存');
  const panel = screen.getByLabelText('数据洞察');
  expect(panel).toBeInTheDocument();
  expect(within(panel).getByText('0')).toBeInTheDocument();
  expect(within(panel).getByText('0.0%')).toBeInTheDocument();
});

test('submits the formula bar to the current target cell', async () => {
  render(<App />);
  await screen.findByText('已自动保存');

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
