function getTargetConfig(env = process.env) {
  return {
    platform: env.MINISHEET_TARGET_PLATFORM || env.npm_config_platform || process.platform,
    arch: env.MINISHEET_TARGET_ARCH || env.npm_config_arch || process.arch,
  };
}

function getBackendBinaryName(target) {
  return target.platform === 'win32' ? 'minisheet_server.exe' : 'minisheet_server';
}

function getBackendBuildDirName(target) {
  if (target.platform === process.platform && target.arch === process.arch) {
    return 'build';
  }

  return `build-${target.platform}-${target.arch}`;
}

module.exports = {
  getBackendBinaryName,
  getBackendBuildDirName,
  getTargetConfig,
};
