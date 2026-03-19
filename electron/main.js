const { app, BrowserWindow, Menu, dialog } = require('electron');
const { spawn } = require('child_process');
const http = require('http');
const net = require('net');
const path = require('path');
const {
  getRuntimeIconPath,
} = require('./scripts/icon-assets.cjs');
const {
  buildAppMenuTemplate,
} = require('./scripts/app-menu.cjs');

let backendProcess = null;
let mainWindow = null;

function getBackendBinaryName() {
  return process.platform === 'win32' ? 'minisheet_server.exe' : 'minisheet_server';
}

function getBackendBinaryPath() {
  if (process.env.MINISHEET_SERVER_PATH) {
    return process.env.MINISHEET_SERVER_PATH;
  }

  if (app.isPackaged) {
    return path.join(process.resourcesPath, getBackendBinaryName());
  }

  return path.resolve(__dirname, '../backend/build', getBackendBinaryName());
}

function getRendererEntry() {
  if (process.env.MINISHEET_RENDERER_URL) {
    return { type: 'url', value: process.env.MINISHEET_RENDERER_URL };
  }

  if (app.isPackaged) {
    return { type: 'file', value: path.join(process.resourcesPath, 'build', 'index.html') };
  }

  return { type: 'file', value: path.resolve(__dirname, '../frontend/build/index.html') };
}

function findAvailablePort() {
  return new Promise((resolve, reject) => {
    const probe = net.createServer();

    probe.once('error', reject);
    probe.listen(0, '127.0.0.1', () => {
      const address = probe.address();
      const port = typeof address === 'object' && address ? address.port : 0;
      probe.close((error) => {
        if (error) {
          reject(error);
          return;
        }
        resolve(port);
      });
    });
  });
}

function waitForBackend(apiBase, timeoutMs = 15000) {
  return new Promise((resolve, reject) => {
    const startedAt = Date.now();

    const tryConnect = () => {
      const request = http.get(`${apiBase}/api/snapshot`, (response) => {
        response.resume();
        if (response.statusCode === 200) {
          resolve();
          return;
        }

        if (Date.now() - startedAt > timeoutMs) {
          reject(new Error(`Backend health check failed with status ${response.statusCode}`));
          return;
        }

        setTimeout(tryConnect, 250);
      });

      request.on('error', () => {
        if (Date.now() - startedAt > timeoutMs) {
          reject(new Error('Timed out waiting for Minisheet backend to start'));
          return;
        }

        setTimeout(tryConnect, 250);
      });
    };

    tryConnect();
  });
}

async function startBackend() {
  const port = await findAvailablePort();
  const apiBase = `http://127.0.0.1:${port}`;
  const backendPath = getBackendBinaryPath();

  await new Promise((resolve, reject) => {
    let settled = false;
    let stderr = '';

    backendProcess = spawn(backendPath, [String(port)], {
      stdio: ['ignore', 'pipe', 'pipe'],
      windowsHide: true,
    });

    backendProcess.stdout?.on('data', (chunk) => {
      process.stdout.write(`[backend] ${chunk}`);
    });

    backendProcess.stderr?.on('data', (chunk) => {
      const text = chunk.toString();
      stderr += text;
      process.stderr.write(`[backend] ${text}`);
    });

    backendProcess.once('error', (error) => {
      if (settled) {
        return;
      }
      settled = true;
      reject(error);
    });

    backendProcess.once('exit', (code) => {
      if (settled) {
        return;
      }
      settled = true;
      reject(new Error(stderr || `Minisheet backend exited early with code ${code}`));
    });

    waitForBackend(apiBase)
      .then(() => {
        if (settled) {
          return;
        }
        settled = true;
        resolve();
      })
      .catch((error) => {
        if (settled) {
          return;
        }
        settled = true;
        reject(new Error(stderr ? `${error.message}\n${stderr}` : error.message));
      });
  });

  return apiBase;
}

function stopBackend() {
  if (!backendProcess || backendProcess.killed) {
    return;
  }

  backendProcess.kill();
  backendProcess = null;
}

async function createMainWindow(apiBase) {
  const entry = getRendererEntry();
  mainWindow = new BrowserWindow({
    width: 1280,
    height: 860,
    icon: getRuntimeIconPath(),
    show: false,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      contextIsolation: true,
      nodeIntegration: false,
      additionalArguments: [`--api-base=${apiBase}`],
    },
  });

  mainWindow.on('closed', () => {
    mainWindow = null;
  });

  mainWindow.once('ready-to-show', () => {
    mainWindow?.show();
  });

  if (entry.type === 'url') {
    await mainWindow.loadURL(entry.value);
    return;
  }

  await mainWindow.loadFile(entry.value);
}

app.on('before-quit', () => {
  stopBackend();
});

app.whenReady()
  .then(async () => {
    Menu.setApplicationMenu(
      Menu.buildFromTemplate(buildAppMenuTemplate(process.platform, app.name)),
    );
    const apiBase = await startBackend();
    await createMainWindow(apiBase);
  })
  .catch((error) => {
    dialog.showErrorBox('Minisheet 启动失败', error instanceof Error ? error.message : String(error));
    app.quit();
  });

app.on('window-all-closed', () => {
  app.quit();
});
