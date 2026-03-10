import React from 'react';
import ReactDOM from 'react-dom/client';

import '@glideapps/glide-data-grid/dist/index.css';

import App from './App';
import './index.css';

const rootElement = document.getElementById('root');

if (!rootElement) {
  throw new Error('Missing root element');
}

ReactDOM.createRoot(rootElement).render(
  <React.StrictMode>
    <App />
  </React.StrictMode>,
);
