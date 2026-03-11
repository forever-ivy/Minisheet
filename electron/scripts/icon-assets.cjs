const path = require('path');

const electronDir = path.resolve(__dirname, '..');
const rootDir = path.resolve(electronDir, '..');
const assetsDir = path.join(electronDir, 'assets');

function getIconSourcePath() {
  return path.join(rootDir, 'frontend', 'public', 'Minisheet.png');
}

function getRuntimeIconPath() {
  return path.join(assetsDir, 'Minisheet.png');
}

function getMacIconPath() {
  return path.join(assetsDir, 'Minisheet.icns');
}

function getWindowsIconPath() {
  return path.join(assetsDir, 'Minisheet.ico');
}

function getPackagerIconBase() {
  return path.join(assetsDir, 'Minisheet');
}

function getAssetsDir() {
  return assetsDir;
}

module.exports = {
  getAssetsDir,
  getIconSourcePath,
  getMacIconPath,
  getPackagerIconBase,
  getRuntimeIconPath,
  getWindowsIconPath,
};
