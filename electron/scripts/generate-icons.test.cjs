const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('fs');

const { generateIcons } = require('./generate-icons.cjs');
const { getMacIconPath } = require('./icon-assets.cjs');

test('generateIcons creates a macOS icns asset on darwin hosts', (t) => {
  if (process.platform !== 'darwin') {
    t.skip('macOS-only icon generation');
    return;
  }

  fs.rmSync(getMacIconPath(), { force: true });

  assert.doesNotThrow(() => {
    generateIcons();
  });
  assert.equal(fs.existsSync(getMacIconPath()), true);
});
