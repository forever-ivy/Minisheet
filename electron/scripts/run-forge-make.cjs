const fs = require('fs');
const { spawnSync } = require('child_process');
const {
  getPackagerCacheRoot,
  withPackagerToolPath,
} = require('./package-env.cjs');

const electronForgeBin = require.resolve('@electron-forge/cli/dist/electron-forge.js');
const args = [electronForgeBin, 'make', ...process.argv.slice(2)];
const env = withPackagerToolPath();

for (const dir of [
  getPackagerCacheRoot(),
  env.XDG_CONFIG_HOME,
  env.XDG_DATA_HOME,
  env.NUGET_PACKAGES,
  env.WINEPREFIX,
]) {
  fs.mkdirSync(dir, { recursive: true });
}

const result = spawnSync(process.execPath, args, {
  stdio: 'inherit',
  env,
});

if (result.status !== 0) {
  process.exit(result.status ?? 1);
}
