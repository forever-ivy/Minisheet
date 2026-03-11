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

const target = getTargetConfig();
const backendBinaryName = getBackendBinaryName(target);
const backendBuildDirName = getBackendBuildDirName(target);
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
    executableName: 'Minisheet',
    icon: getPackagerIconBase(),
    extraResource,
  },
  makers: [
    {
      name: '@electron-forge/maker-zip',
      platforms: ['darwin', 'win32'],
    },
  ],
};
