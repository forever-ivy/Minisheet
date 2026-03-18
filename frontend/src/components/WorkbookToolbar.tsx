import { Download, type LucideIcon } from 'lucide-react';

import { tabLabels } from '../app/workbookUtils';

type ToolbarAction = {
  key: string;
  label: string;
  icon: LucideIcon;
  onClick: () => void;
};

type WorkbookToolbarProps = {
  activeTab: string;
  onTabChange: (nextTab: 'home' | 'insert') => void;
  visibleActions: ToolbarAction[];
  onExportCsv: () => void;
};

export function WorkbookToolbar({
  activeTab,
  onTabChange,
  visibleActions,
  onExportCsv,
}: WorkbookToolbarProps) {
  return (
    <div className="toolbar-container">
      <div className="pill-row">
        <div className="pill-group">
          {tabLabels.map((tab) => (
            <button
              key={tab.value}
              type="button"
              className={`pill-btn ${activeTab === tab.value ? 'active' : ''}`}
              onClick={() => onTabChange(tab.value)}
            >
              {tab.label}
            </button>
          ))}
        </div>

        <div className="action-pills" aria-label={`${activeTab} 工具区`}>
          {visibleActions.map(({ key, label, icon: Icon, onClick }) => (
            <button key={key} type="button" className="pill-btn" onClick={onClick}>
              <Icon className="icon icon-small" />
              {label}
            </button>
          ))}
        </div>

        <button className="pill-btn primary" type="button" onClick={onExportCsv}>
          <Download className="icon icon-small" />
          导出 CSV
        </button>
      </div>
    </div>
  );
}
