const test = require('node:test');
const assert = require('node:assert/strict');
const path = require('path');

const {
  getIconSourcePath,
  getRuntimeIconPath,
  getMacIconPath,
  getWindowsIconPath,
  getPackagerIconBase,
} = require('./icon-assets.cjs');

const forgeConfig = require('../forge.config.js');

const rootDir = path.resolve(__dirname, '../..');
const electronDir = path.resolve(__dirname, '..');

test('resolves icon source and generated asset paths from a single PNG source', () => {
  assert.equal(getIconSourcePath(), path.join(rootDir, 'frontend', 'public', 'Minisheet.png'));
  assert.equal(getRuntimeIconPath(), path.join(electronDir, 'assets', 'Minisheet.png'));
  assert.equal(getMacIconPath(), path.join(electronDir, 'assets', 'Minisheet.icns'));
  assert.equal(getWindowsIconPath(), path.join(electronDir, 'assets', 'Minisheet.ico'));
  assert.equal(getPackagerIconBase(), path.join(electronDir, 'assets', 'Minisheet'));
});

test('forge packager config points to the generated icon base path', () => {
  assert.equal(forgeConfig.packagerConfig.icon, getPackagerIconBase());
});
