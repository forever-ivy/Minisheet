import { BarChart3, Copy, Download, MoreHorizontal, Sparkles } from 'lucide-react';
import { useEffect, useRef, useState } from 'react';

import {
  formatGrowthPct,
  formatInsightValue,
  type InsightMetric,
  type SelectionStats,
} from '../spreadsheet/adapter';

type InsightPanelProps = {
  metric: InsightMetric;
  stats: SelectionStats;
  rangeRef: string;
  visible: boolean;
  onToggleVisible: () => void;
  onExportDat: () => void;
};

function copyTextToClipboard(text: string) {
  if (navigator.clipboard?.writeText) {
    return navigator.clipboard.writeText(text);
  }

  const textarea = document.createElement('textarea');
  textarea.value = text;
  textarea.style.position = 'fixed';
  textarea.style.left = '-9999px';
  document.body.appendChild(textarea);
  textarea.select();
  document.execCommand('copy');
  document.body.removeChild(textarea);
  return Promise.resolve();
}

export function InsightPanel({ metric, stats, rangeRef, visible, onToggleVisible, onExportDat }: InsightPanelProps) {
  const [menuOpen, setMenuOpen] = useState(false);
  const menuRef = useRef<HTMLDivElement | null>(null);
  const maxValue = Math.max(...metric.values.map((value) => Math.abs(value)), 1);
  const details = `范围: ${rangeRef}\n总和: ${formatInsightValue(stats.sum)}\n平均: ${
    stats.count === 0 ? '-' : formatInsightValue(stats.avg)
  }\n计数: ${stats.count}`;

  useEffect(() => {
    if (!visible) {
      setMenuOpen(false);
      return;
    }

    if (!menuOpen) {
      return;
    }

    const handlePointerDown = (event: MouseEvent) => {
      if (!menuRef.current) {
        return;
      }
      if (event.target instanceof Node && menuRef.current.contains(event.target)) {
        return;
      }
      setMenuOpen(false);
    };

    document.addEventListener('mousedown', handlePointerDown);
    return () => document.removeEventListener('mousedown', handlePointerDown);
  }, [menuOpen, visible]);

  if (!visible) {
    return null;
  }

  return (
    <aside className="floating-panel" aria-label="数据洞察">
      <div className="panel-header">
        <div className="panel-title">
          <BarChart3 className="icon icon-emerald" />
          数据洞察
        </div>
        <div ref={menuRef} className={`panel-menu ${menuOpen ? 'is-open' : ''}`}>
          <button
            className="glass-btn"
            type="button"
            aria-label="洞察更多操作"
            onClick={() => setMenuOpen((current) => !current)}
          >
            <MoreHorizontal className="icon icon-small" />
          </button>
          <div className="panel-popover" role="menu" aria-label="洞察菜单">
            <button
              type="button"
              className="panel-popover-item"
              role="menuitem"
              onClick={() => {
                setMenuOpen(false);
                void copyTextToClipboard(details);
              }}
            >
              <Copy className="icon icon-small" />
              复制统计
            </button>
            <button
              type="button"
              className="panel-popover-item"
              role="menuitem"
              onClick={() => {
                setMenuOpen(false);
                onToggleVisible();
              }}
            >
              <Sparkles className="icon icon-small" />
              切换洞察
            </button>
            <button
              type="button"
              className="panel-popover-item"
              role="menuitem"
              onClick={() => {
                setMenuOpen(false);
                onExportDat();
              }}
            >
              <Download className="icon icon-small" />
              导出 DAT
            </button>
          </div>
        </div>
      </div>

      <div className="mini-chart" aria-hidden="true">
        {metric.values.map((value, index) => {
          const height = maxValue === 0 ? 24 : Math.max(24, Math.round((Math.abs(value) / maxValue) * 100));
          const isPeak = Math.abs(value) === maxValue && maxValue !== 0;
          return (
            <div
              key={`${index}-${value}`}
              className={`bar ${isPeak ? 'active' : ''}`}
              style={{ height: `${height}%` }}
            />
          );
        })}
      </div>

      <div className="stat-row">
        <div>
          <div className="stat-label">总和</div>
          <div className="stat-value">{formatInsightValue(metric.sum)}</div>
        </div>
        <div>
          <div className="stat-label stat-label-right">变化</div>
          <div className="text-emerald text-compact">{formatGrowthPct(metric.growthPct)}</div>
        </div>
      </div>
    </aside>
  );
}
