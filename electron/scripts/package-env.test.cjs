const test = require('node:test');
const assert = require('node:assert/strict');
const path = require('path');

const {
  getPackagerCacheRoot,
  getPackagerToolBinDir,
  withPackagerToolPath,
} = require('./package-env.cjs');

test('packager PATH prepends the local tool shim directory', () => {
  const env = withPackagerToolPath({
    PATH: '/usr/bin:/bin',
  });

  assert.equal(env.PATH, `${getPackagerToolBinDir()}:/usr/bin:/bin`);
  assert.equal(env.XDG_CONFIG_HOME, path.join(getPackagerCacheRoot(), 'xdg-config'));
  assert.equal(env.XDG_DATA_HOME, path.join(getPackagerCacheRoot(), 'xdg-data'));
  assert.equal(env.NUGET_PACKAGES, path.join(getPackagerCacheRoot(), 'nuget-packages'));
  assert.equal(env.WINEPREFIX, path.join(getPackagerCacheRoot(), 'wineprefix'));
});

test('packager tool shim directory points at electron/scripts/bin', () => {
  assert.equal(
    getPackagerToolBinDir(),
    path.join(path.resolve(__dirname), 'bin'),
  );
});
