import { ensurePortalRoot } from './portal';

afterEach(() => {
  document.getElementById('portal')?.remove();
});

test('creates a portal root when one is missing', () => {
  expect(document.getElementById('portal')).toBeNull();

  const portal = ensurePortalRoot();

  expect(portal.id).toBe('portal');
  expect(document.body.lastElementChild).toBe(portal);
});

test('reuses the existing portal root when it is already present', () => {
  const existing = document.createElement('div');
  existing.id = 'portal';
  document.body.appendChild(existing);

  const portal = ensurePortalRoot();

  expect(portal).toBe(existing);
  expect(document.querySelectorAll('#portal')).toHaveLength(1);
});
