const test = require('node:test');
const assert = require('node:assert/strict');

const {
  getWindowsRuntimeDllPaths,
} = require('./windows-runtime.cjs');

test('returns required mingw runtime DLLs for win32 packaging', () => {
  const paths = getWindowsRuntimeDllPaths({
    MINGW_CXX: 'x86_64-w64-mingw32-g++',
  });

  const names = paths.map((value) => value.split(/[\\/]/).pop()).sort();
  assert.deepEqual(names, [
    'libgcc_s_seh-1.dll',
    'libstdc++-6.dll',
    'libwinpthread-1.dll',
  ]);
});
