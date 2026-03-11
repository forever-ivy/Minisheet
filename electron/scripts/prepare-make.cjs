const fs = require('fs');
const path = require('path');
const { spawnSync } = require('child_process');
const {
  getBackendBinaryName,
  getBackendBuildDirName,
  getTargetConfig,
} = require('./target-config.cjs');
const {
  getWindowsRuntimeDllPaths,
} = require('./windows-runtime.cjs');

const rootDir = path.resolve(__dirname, '../..');
const frontendIndexPath = path.join(rootDir, 'frontend', 'build', 'index.html');
const target = getTargetConfig();
const backendBinaryPath = path.join(rootDir, 'backend', getBackendBuildDirName(target), getBackendBinaryName(target));
const npmCommand = process.platform === 'win32' ? 'npm.cmd' : 'npm';

function run(scriptName) {
  const result = spawnSync(npmCommand, ['run', scriptName], {
    cwd: path.resolve(__dirname, '..'),
    stdio: 'inherit',
  });

  if (result.status !== 0) {
    process.exit(result.status ?? 1);
  }
}

run('build:renderer');
run('build:icons');
run('build:backend');

if (!fs.existsSync(frontendIndexPath)) {
  console.error(`Missing renderer build output: ${frontendIndexPath}`);
  process.exit(1);
}

if (!fs.existsSync(backendBinaryPath)) {
  console.error(`Missing backend binary: ${backendBinaryPath}`);
  process.exit(1);
}

if (target.platform === 'win32') {
  for (const dllPath of getWindowsRuntimeDllPaths()) {
    if (!fs.existsSync(dllPath)) {
      console.error(`Missing Windows runtime DLL: ${dllPath}`);
      process.exit(1);
    }
  }
}
