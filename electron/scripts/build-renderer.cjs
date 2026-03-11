const { spawnSync } = require('child_process');
const path = require('path');

const rootDir = path.resolve(__dirname, '../..');
const frontendDir = path.join(rootDir, 'frontend');
const npmCommand = process.platform === 'win32' ? 'npm.cmd' : 'npm';

const result = spawnSync(npmCommand, ['--prefix', frontendDir, 'run', 'build'], {
  cwd: rootDir,
  stdio: 'inherit',
  env: {
    ...process.env,
    PUBLIC_URL: './',
  },
});

if (result.status !== 0) {
  process.exit(result.status ?? 1);
}
