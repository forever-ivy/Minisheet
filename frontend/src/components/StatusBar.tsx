import { Sigma } from 'lucide-react';

import { formatInsightValue, type SelectionStats } from '../spreadsheet/adapter';

type StatusBarProps = {
  status: string;
  workbookTitle: string;
  rangeRef: string;
  stats: SelectionStats;
  nonEmptyCount: number;
  computeMs: number;
};

export function StatusBar({
  status,
  workbookTitle,
  rangeRef,
  stats,
  nonEmptyCount,
  computeMs,
}: StatusBarProps) {
  return (
    <footer className="status-bar" aria-label="状态栏">
      <div className="status-left">
        <span className="status-dot" aria-hidden="true" />
        <span className="status-text">{status}</span>
        <span className="status-divider" aria-hidden="true">
          ·
        </span>
        <span className="status-title">{workbookTitle || '未命名'}</span>
      </div>

      <div className="status-center" aria-label="选区统计">
        <span className="status-range">{rangeRef}</span>
        <span className="status-kpi">
          <Sigma className="icon icon-small icon-emerald" />
          {formatInsightValue(stats.sum)}
        </span>
        <span className="status-kpi">
          平均 {stats.count === 0 ? '-' : formatInsightValue(stats.avg)}
        </span>
        <span className="status-kpi">计数 {stats.count}</span>
      </div>

      <div className="status-right">
        <span>{nonEmptyCount} 个非空单元格</span>
        <span>{computeMs.toFixed(2)} ms</span>
      </div>
    </footer>
  );
}

