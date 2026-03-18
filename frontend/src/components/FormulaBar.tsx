import { Check } from 'lucide-react';
import { type ChangeEvent, type FormEvent, type RefObject } from 'react';

type FormulaBarProps = {
  activeCellId: string;
  formulaDraft: string;
  inputRef: RefObject<HTMLInputElement>;
  onSubmit: (event?: FormEvent) => void;
  onFocus: () => void;
  onChange: (event: ChangeEvent<HTMLInputElement>) => void;
};

export function FormulaBar({
  activeCellId,
  formulaDraft,
  inputRef,
  onSubmit,
  onFocus,
  onChange,
}: FormulaBarProps) {
  return (
    <div className="formula-bar">
      <div className="cell-ref">{activeCellId}</div>
      <div className="fx-icon">fx</div>
      <form className="formula-form" onSubmit={onSubmit}>
        <input
          ref={inputRef}
          className="formula-input"
          type="text"
          aria-label="公式栏"
          value={formulaDraft}
          onFocus={onFocus}
          onChange={onChange}
          placeholder="输入内容或公式"
        />
        <button type="submit" className="formula-submit" aria-label="提交公式">
          <Check className="icon icon-small" />
        </button>
      </form>
    </div>
  );
}
