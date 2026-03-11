const { spawnSync } = require('child_process');
const path = require('path');
const {
  getBackendBuildDirName,
  getTargetConfig,
} = require('./target-config.cjs');

const rootDir = path.resolve(__dirname, '../..');
const backendDir = path.join(rootDir, 'backend');
const target = getTargetConfig();
const backendBuildDir = path.join(backendDir, getBackendBuildDirName(target));

function run(command, args) {
  const result = spawnSync(command, args, {
    cwd: rootDir,
    stdio: 'inherit',
  });

  if (result.status !== 0) {
    process.exit(result.status ?? 1);
  }
}

function buildForWindows() {
  const cCompiler = process.env.MINGW_CC || 'x86_64-w64-mingw32-gcc';
  const cxxCompiler = process.env.MINGW_CXX || 'x86_64-w64-mingw32-g++';

  run('cmake', [
    '-S',
    backendDir,
    '-B',
    backendBuildDir,
    '-DCMAKE_SYSTEM_NAME=Windows',
    '-DCMAKE_SYSTEM_PROCESSOR=x86_64',
    `-DCMAKE_C_COMPILER=${cCompiler}`,
    `-DCMAKE_CXX_COMPILER=${cxxCompiler}`,
  ]);
  run('cmake', ['--build', backendBuildDir]);
}

if (target.platform === 'win32' && process.platform !== 'win32') {
  buildForWindows();
  process.exit(0);
}

run('cmake', ['-S', backendDir, '-B', backendBuildDir]);
run('cmake', ['--build', backendBuildDir]);
