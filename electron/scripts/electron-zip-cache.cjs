const fs = require('fs');
const os = require('os');
const path = require('path');

function getDefaultElectronCacheDir(platform = process.platform, env = process.env) {
  if (platform === 'darwin') {
    return path.join(os.homedir(), 'Library', 'Caches', 'electron');
  }

  if (platform === 'win32') {
    const localAppData = env.LOCALAPPDATA || path.join(os.homedir(), 'AppData', 'Local');
    return path.join(localAppData, 'electron', 'Cache');
  }

  return path.join(os.homedir(), '.cache', 'electron');
}

function getElectronZipDir(options = {}) {
  const platform = options.platform || process.platform;
  const arch = options.arch || process.arch;
  const version = options.version;
  const env = options.env || process.env;
  const cachePlatform = options.cachePlatform || process.platform;
  const preferredDir =
    env.ELECTRON_ZIP_DIR || env.ELECTRON_CACHE || getDefaultElectronCacheDir(cachePlatform, env);
  const zipName = version ? `electron-v${version}-${platform}-${arch}.zip` : undefined;

  if (!fs.existsSync(preferredDir)) {
    return undefined;
  }

  if (!zipName) {
    return preferredDir;
  }

  if (fs.existsSync(path.join(preferredDir, zipName))) {
    return preferredDir;
  }

  for (const entry of fs.readdirSync(preferredDir, { withFileTypes: true })) {
    if (!entry.isDirectory()) {
      continue;
    }

    const candidateDir = path.join(preferredDir, entry.name);
    if (fs.existsSync(path.join(candidateDir, zipName))) {
      return candidateDir;
    }
  }

  return undefined;
}

module.exports = {
  getDefaultElectronCacheDir,
  getElectronZipDir,
};
