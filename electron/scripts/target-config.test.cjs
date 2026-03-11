const test = require('node:test');
const assert = require('node:assert/strict');

const {
  getTargetConfig,
  getBackendBinaryName,
  getBackendBuildDirName,
} = require('./target-config.cjs');

test('defaults to the current platform when no target env is provided', () => {
  const config = getTargetConfig({});

  assert.equal(config.platform, process.platform);
  assert.equal(config.arch, process.arch);
});

test('uses explicit Windows target values for packaging helpers', () => {
  const config = getTargetConfig({
    MINISHEET_TARGET_PLATFORM: 'win32',
    MINISHEET_TARGET_ARCH: 'x64',
  });

  assert.deepEqual(config, {
    platform: 'win32',
    arch: 'x64',
  });
  assert.equal(getBackendBinaryName(config), 'minisheet_server.exe');
  assert.equal(getBackendBuildDirName(config), 'build-win32-x64');
});
