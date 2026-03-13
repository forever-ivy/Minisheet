const os = require('os');
const path = require('path');

function getPackagerToolBinDir() {
  return path.join(__dirname, 'bin');
}

function getPackagerCacheRoot() {
  return path.join(os.tmpdir(), 'minisheet-packager-cache');
}

function withPackagerToolPath(env = process.env) {
  return {
    ...env,
    PATH: `${getPackagerToolBinDir()}:${env.PATH || ''}`,
    XDG_CONFIG_HOME: path.join(getPackagerCacheRoot(), 'xdg-config'),
    XDG_DATA_HOME: path.join(getPackagerCacheRoot(), 'xdg-data'),
    NUGET_PACKAGES: path.join(getPackagerCacheRoot(), 'nuget-packages'),
    WINEPREFIX: path.join(getPackagerCacheRoot(), 'wineprefix'),
  };
}

module.exports = {
  getPackagerCacheRoot,
  getPackagerToolBinDir,
  withPackagerToolPath,
};
