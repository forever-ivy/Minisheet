import { X } from 'lucide-react';

type HelpModalProps = {
  open: boolean;
  onClose: () => void;
};

export function HelpModal({ open, onClose }: HelpModalProps) {
  if (!open) {
    return null;
  }

  return (
    <div className="modal-backdrop" role="presentation" onMouseDown={onClose}>
      <section
        className="modal-card"
        role="dialog"
        aria-modal="true"
        aria-label="帮助"
        onMouseDown={(event) => event.stopPropagation()}
      >
        <header className="modal-header">
          <div className="modal-title">帮助</div>
          <button type="button" className="modal-close" aria-label="关闭帮助" onClick={onClose}>
            <X className="icon icon-small" />
          </button>
        </header>

        <div className="modal-body">
          <div className="modal-section">
            <div className="modal-section-title">公式</div>
            <div className="modal-code">=A1+B1</div>
            <div className="modal-code">=SUM(B2:C4)</div>
            <div className="modal-code">=AVG(B2:C4)</div>
            <div className="modal-code">=SQRT(B2)</div>
            <div className="modal-code">=ABS(B2)</div>
          </div>

          <div className="modal-section">
            <div className="modal-section-title">操作</div>
            <div className="modal-row">回车/对勾：提交公式到当前单元格</div>
            <div className="modal-row">拖拽选区：在公式编辑时插入引用</div>
            <div className="modal-row">工具按钮：先点函数，再点选/拖拽区域自动计算</div>
            <div className="modal-row">Esc：取消等待选择</div>
          </div>
        </div>
      </section>
    </div>
  );
}

