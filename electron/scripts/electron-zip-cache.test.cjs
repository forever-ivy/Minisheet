const test = require('node:test');
const assert = require('node:assert/strict');
const os = require('os');
const path = require('path');

const forgeConfig = require('../forge.config.js');
const { getTargetConfig } = require('./target-config.cjs');
const { getElectronZipDir } = require('./electron-zip-cache.cjs');

test('forge packager prefers the local electron zip cache when available', () => {
  const expectedCacheDir = path.join(os.homedir(), 'Library', 'Caches', 'electron');
  const target = getTargetConfig();
  const electronVersion = require('../node_modules/electron/package.json').version;
  const expectedZipName = `electron-v${electronVersion}-${target.platform}-${target.arch}.zip`;

  assert.match(forgeConfig.packagerConfig.electronZipDir, new RegExp(`^${expectedCacheDir}`));
  assert.equal(
    path.basename(path.join(forgeConfig.packagerConfig.electronZipDir, expectedZipName)),
    expectedZipName,
  );
  assert.equal(
    require('fs').existsSync(path.join(forgeConfig.packagerConfig.electronZipDir, expectedZipName)),
    true,
  );
});

test('electron zip cache lookup uses the host cache root for cross-platform packaging', () => {
  const resolvedZipDir = getElectronZipDir({
    version: '41.0.0',
    platform: 'win32',
    arch: 'x64',
  });

  assert.match(resolvedZipDir, new RegExp(`^${path.join(os.homedir(), 'Library', 'Caches', 'electron')}`));
  assert.equal(
    require('fs').existsSync(path.join(resolvedZipDir, 'electron-v41.0.0-win32-x64.zip')),
    true,
  );
});
