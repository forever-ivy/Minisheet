const fs = require('fs');
const path = require('path');
const { spawnSync } = require('child_process');

const REQUIRED_DLLS = [
  'libgcc_s_seh-1.dll',
  'libstdc++-6.dll',
  'libwinpthread-1.dll',
];

function runAndRead(command, args) {
  const result = spawnSync(command, args, {
    encoding: 'utf8',
  });

  if (result.status !== 0) {
    return '';
  }

  return result.stdout.trim();
}

function resolveCommandPath(command) {
  if (path.isAbsolute(command) && fs.existsSync(command)) {
    return command;
  }

  const locator = process.platform === 'win32' ? 'where' : 'which';
  const output = runAndRead(locator, [command]);
  if (!output) {
    return command;
  }

  const firstLine = output.split(/\r?\n/).find(Boolean);
  if (!firstLine) {
    return command;
  }

  return fs.realpathSync(firstLine.trim());
}

function resolveFromCompiler(compilerPath, dllName) {
  const output = runAndRead(compilerPath, [`-print-file-name=${dllName}`]);
  if (!output || output === dllName) {
    return '';
  }

  const resolvedPath = path.resolve(output);
  return fs.existsSync(resolvedPath) ? resolvedPath : '';
}

function resolveFromToolchain(compilerPath, dllName) {
  const compilerDir = path.dirname(compilerPath);
  const toolchainRoot = path.resolve(compilerDir, '..');
  const candidates = [
    path.join(toolchainRoot, 'x86_64-w64-mingw32', 'lib', dllName),
    path.join(toolchainRoot, 'x86_64-w64-mingw32', 'bin', dllName),
  ];

  return candidates.find((value) => fs.existsSync(value)) || '';
}

function getWindowsRuntimeDllPaths(env = process.env) {
  const compiler = env.MINGW_CXX || 'x86_64-w64-mingw32-g++';
  const compilerPath = resolveCommandPath(compiler);

  return REQUIRED_DLLS.map((dllName) => {
    const fromCompiler = resolveFromCompiler(compilerPath, dllName);
    if (fromCompiler) {
      return fromCompiler;
    }

    const fromToolchain = resolveFromToolchain(compilerPath, dllName);
    if (fromToolchain) {
      return fromToolchain;
    }

    throw new Error(`Missing Windows runtime DLL: ${dllName}`);
  });
}

module.exports = {
  getWindowsRuntimeDllPaths,
};
