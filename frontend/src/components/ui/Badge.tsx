import { HTMLAttributes } from 'react';

type BadgeVariant = 'default' | 'secondary' | 'outline';

type BadgeProps = HTMLAttributes<HTMLSpanElement> & {
  variant?: BadgeVariant;
};

function joinClassNames(...values: Array<string | undefined>) {
  return values.filter(Boolean).join(' ');
}

export function Badge({ className, variant = 'default', ...props }: BadgeProps) {
  return <span className={joinClassNames('ui-badge', `ui-badge--${variant}`, className)} {...props} />;
}
