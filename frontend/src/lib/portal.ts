export function ensurePortalRoot(): HTMLDivElement {
  const existing = document.getElementById('portal');
  if (existing instanceof HTMLDivElement) {
    return existing;
  }

  const portal = document.createElement('div');
  portal.id = 'portal';
  document.body.appendChild(portal);
  return portal;
}
