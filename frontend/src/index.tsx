import React from 'react';
import ReactDOM from 'react-dom/client';

import '@glideapps/glide-data-grid/dist/index.css';

import App from './App';
import './index.css';
import { ensurePortalRoot } from './lib/portal';

const rootElement = document.getElementById('root');

if (!rootElement) {
  throw new Error('Missing root element');
}

ensurePortalRoot();

ReactDOM.createRoot(rootElement).render(
  <React.StrictMode>
    <App />
  </React.StrictMode>,
);
