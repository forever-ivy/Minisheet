export {};

declare global {
  interface Window {
    desktopConfig?: {
      apiBase?: string;
    };
  }
}
