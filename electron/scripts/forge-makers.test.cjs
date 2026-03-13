const test = require('node:test');
const assert = require('node:assert/strict');

function loadForgeConfig() {
  delete require.cache[require.resolve('../forge.config.js')];
  return require('../forge.config.js');
}

test('forge config keeps zip packaging and adds a Windows squirrel maker', () => {
  const forgeConfig = loadForgeConfig();
  const makersByName = new Map(forgeConfig.makers.map((maker) => [maker.name, maker]));

  assert.deepEqual([...makersByName.keys()].sort(), [
    '@electron-forge/maker-squirrel',
    '@electron-forge/maker-zip',
  ]);
  assert.deepEqual(makersByName.get('@electron-forge/maker-zip').platforms, ['darwin', 'win32']);
  assert.deepEqual(makersByName.get('@electron-forge/maker-squirrel').platforms, ['win32']);
});

test('desktop package metadata required for installer packaging is present', () => {
  const pkg = require('../package.json');

  assert.equal(typeof pkg.description, 'string');
  assert.notEqual(pkg.description.trim(), '');
  assert.equal(typeof pkg.author, 'string');
  assert.notEqual(pkg.author.trim(), '');
});
