const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawnSync } = require('child_process');

const rootDir = path.resolve(__dirname, '../..');

test('minisheet_server.cpp compiles with the configured Windows toolchain', (t) => {
  const compiler = process.env.MINGW_CXX || 'x86_64-w64-mingw32-g++';
  const compilerCheck = spawnSync(compiler, ['--version'], {
    encoding: 'utf8',
  });

  if (compilerCheck.status !== 0) {
    t.skip('mingw-w64 compiler is unavailable');
    return;
  }

  const outputDir = fs.mkdtempSync(path.join(os.tmpdir(), 'minisheet-win-compile-'));
  const objectPath = path.join(outputDir, 'minisheet_server.obj');
  const compile = spawnSync(compiler, [
    '-std=c++17',
    '-Ibackend/include',
    '-Ibackend/third_party/tinyexpr',
    '-Ivendor',
    '-c',
    'backend/app/minisheet_server.cpp',
    '-o',
    objectPath,
  ], {
    cwd: rootDir,
    encoding: 'utf8',
  });

  assert.equal(
    compile.status,
    0,
    compile.stderr || compile.stdout || 'Windows cross-compile failed',
  );
  assert.equal(fs.existsSync(objectPath), true);
});
