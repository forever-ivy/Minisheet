const fs = require('fs');
const os = require('os');
const path = require('path');
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

  const tempDir = fs.mkdtempSync(path.join(os.tmpdir(), 'minisheet-iconset-'));
  const iconsetDir = path.join(tempDir, 'Minisheet.iconset');
  const sourcePath = getIconSourcePath();
  const macIconPath = getMacIconPath();
  const sizes = [
    ['icon_16x16.png', 16],
    ['icon_16x16@2x.png', 32],
    ['icon_32x32.png', 32],
    ['icon_32x32@2x.png', 64],
    ['icon_128x128.png', 128],
    ['icon_128x128@2x.png', 256],
    ['icon_256x256.png', 256],
    ['icon_256x256@2x.png', 512],
    ['icon_512x512.png', 512],
    ['icon_512x512@2x.png', 1024],
  ];

  fs.mkdirSync(iconsetDir, { recursive: true });

  try {
    for (const [filename, size] of sizes) {
      run('sips', ['-z', String(size), String(size), sourcePath, '--out', path.join(iconsetDir, filename)]);
    }

    run('iconutil', ['-c', 'icns', iconsetDir, '-o', macIconPath]);
  } finally {
    fs.rmSync(tempDir, { recursive: true, force: true });
  }
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
