import { Badge } from './ui/Badge';

type AppHeaderProps = {
  onHelp: () => void;
  onLoadDat: () => void;
  onExportDat: () => void;
};

export function AppHeader({ onHelp, onLoadDat, onExportDat }: AppHeaderProps) {
  return (
    <header className="app-header">
      <div className="brand">
        <div className="brand-icon" />
        <span>MiniSheet</span>
      </div>

      <div className="header-meta">
        <div className="user-actions">
          <button type="button" className="badge-action" onClick={onHelp}>
            <Badge variant="secondary">帮助</Badge>
          </button>
          <button type="button" className="badge-action" onClick={onLoadDat}>
            <Badge variant="secondary">载入 DAT</Badge>
          </button>
          <button type="button" className="badge-action" onClick={onExportDat}>
            <Badge variant="secondary">导出 DAT</Badge>
          </button>
        </div>
      </div>
    </header>
  );
}
