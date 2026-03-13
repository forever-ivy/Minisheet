const path = require('path');
const {
  getBackendBinaryName,
  getBackendBuildDirName,
  getTargetConfig,
} = require('./scripts/target-config.cjs');
const {
  getWindowsRuntimeDllPaths,
} = require('./scripts/windows-runtime.cjs');
const {
  getPackagerIconBase,
} = require('./scripts/icon-assets.cjs');
const {
  getElectronZipDir,
} = require('./scripts/electron-zip-cache.cjs');

const target = getTargetConfig();
const electronVersion = require('electron/package.json').version;
const backendBinaryName = getBackendBinaryName(target);
const backendBuildDirName = getBackendBuildDirName(target);
const electronZipDir = getElectronZipDir({
  version: electronVersion,
  platform: target.platform,
  arch: target.arch,
});
const extraResource = [
  path.resolve(__dirname, '../frontend/build'),
  path.resolve(__dirname, '../backend', backendBuildDirName, backendBinaryName),
];

if (target.platform === 'win32') {
  extraResource.push(...getWindowsRuntimeDllPaths());
}

module.exports = {
  packagerConfig: {
    asar: true,
    electronVersion,
    executableName: 'Minisheet',
    electronZipDir,
    icon: getPackagerIconBase(),
    extraResource,
  },
  makers: [
    {
      name: '@electron-forge/maker-squirrel',
      platforms: ['win32'],
    },
    {
      name: '@electron-forge/maker-zip',
      platforms: ['darwin', 'win32'],
    },
  ],
};
