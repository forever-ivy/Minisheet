const { contextBridge } = require('electron');

function readApiBase() {
  const entry = process.argv.find((value) => value.startsWith('--api-base='));
  if (!entry) {
    return 'http://127.0.0.1:8080';
  }

  return entry.slice('--api-base='.length);
}

contextBridge.exposeInMainWorld('desktopConfig', Object.freeze({
  apiBase: readApiBase(),
}));
