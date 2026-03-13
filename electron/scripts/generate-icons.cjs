const fs = require('fs');
const { spawnSync } = require('child_process');
const {
  getAssetsDir,
  getIconSourcePath,
  getMacIconPath,
  getRuntimeIconPath,
  getWindowsIconPath,
} = require('./icon-assets.cjs');

function run(command, args, options = {}) {
  const result = spawnSync(command, args, {
    stdio: 'inherit',
    ...options,
  });

  if (result.status !== 0) {
    throw new Error(`${command} exited with code ${result.status ?? 1}`);
  }
}

function ensureAssetsDir() {
  fs.mkdirSync(getAssetsDir(), { recursive: true });
}

function ensureSourceLogoExists() {
  const sourcePath = getIconSourcePath();
  if (!fs.existsSync(sourcePath)) {
    throw new Error(`Missing source logo: ${sourcePath}`);
  }
}

function copyRuntimePng() {
  fs.copyFileSync(getIconSourcePath(), getRuntimeIconPath());
}

function generateWindowsIcon() {
  const sourcePath = getIconSourcePath();
  const outputPath = getWindowsIconPath();
  const script = [
    'from PIL import Image',
    'import sys',
    'source_path, output_path = sys.argv[1], sys.argv[2]',
    'image = Image.open(source_path).convert("RGBA")',
    'sizes = [(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]',
    'image.save(output_path, format="ICO", sizes=sizes)',
  ].join('\n');

  run('python3', ['-c', script, sourcePath, outputPath]);
}

function generateMacIcon() {
  if (process.platform !== 'darwin') {
    return;
  }

  const sourcePath = getIconSourcePath();
  const macIconPath = getMacIconPath();
  const script = [
    'from PIL import Image',
    'import sys',
    'source_path, output_path = sys.argv[1], sys.argv[2]',
    'image = Image.open(source_path).convert("RGBA")',
    'image.save(output_path, format="ICNS")',
  ].join('\n');

  run('python3', ['-c', script, sourcePath, macIconPath]);
}

function generateIcons() {
  ensureSourceLogoExists();
  ensureAssetsDir();
  copyRuntimePng();
  generateWindowsIcon();
  generateMacIcon();
}

if (require.main === module) {
  try {
    generateIcons();
  } catch (error) {
    console.error(error instanceof Error ? error.message : String(error));
    process.exit(1);
  }
}

module.exports = {
  generateIcons,
};
