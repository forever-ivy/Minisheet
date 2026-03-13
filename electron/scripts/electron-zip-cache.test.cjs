const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('fs');
const os = require('os');
const path = require('path');

const { getElectronZipDir } = require('./electron-zip-cache.cjs');

function withEnv(overrides, fn) {
  const originalValues = new Map();

  for (const [key, value] of Object.entries(overrides)) {
    originalValues.set(key, process.env[key]);
    process.env[key] = value;
  }

  try {
    return fn();
  } finally {
    for (const [key, value] of originalValues.entries()) {
      if (value === undefined) {
        delete process.env[key];
      } else {
        process.env[key] = value;
      }
    }
  }
}

function loadForgeConfig() {
  const forgeConfigPath = require.resolve('../forge.config.js');
  delete require.cache[forgeConfigPath];

  try {
    return require(forgeConfigPath);
  } finally {
    delete require.cache[forgeConfigPath];
  }
}

test('forge packager prefers an explicit electron zip cache when available', () => {
  const expectedCacheDir = fs.mkdtempSync(path.join(os.tmpdir(), 'minisheet-electron-cache-'));
  const electronVersion = require('../node_modules/electron/package.json').version;
  const expectedZipName = `electron-v${electronVersion}-win32-x64.zip`;
  fs.writeFileSync(path.join(expectedCacheDir, expectedZipName), '');

  withEnv(
    {
      ELECTRON_ZIP_DIR: expectedCacheDir,
      MINISHEET_TARGET_PLATFORM: 'win32',
      MINISHEET_TARGET_ARCH: 'x64',
    },
    () => {
      const forgeConfig = loadForgeConfig();
      assert.equal(forgeConfig.packagerConfig.electronZipDir, expectedCacheDir);
      assert.equal(
        path.basename(path.join(forgeConfig.packagerConfig.electronZipDir, expectedZipName)),
        expectedZipName,
      );
      assert.equal(
        fs.existsSync(path.join(forgeConfig.packagerConfig.electronZipDir, expectedZipName)),
        true,
      );
    },
  );
});

test('electron zip cache lookup uses an explicit cache root for cross-platform packaging', () => {
  const expectedCacheDir = fs.mkdtempSync(path.join(os.tmpdir(), 'minisheet-electron-cache-'));
  fs.writeFileSync(path.join(expectedCacheDir, 'electron-v41.0.0-win32-x64.zip'), '');

  const resolvedZipDir = getElectronZipDir({
    version: '41.0.0',
    platform: 'win32',
    arch: 'x64',
    env: {
      ELECTRON_ZIP_DIR: expectedCacheDir,
    },
  });

  assert.equal(resolvedZipDir, expectedCacheDir);
  assert.equal(fs.existsSync(path.join(resolvedZipDir, 'electron-v41.0.0-win32-x64.zip')), true);
});
