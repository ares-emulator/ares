import type { SettingDescriptor } from './contracts.js';
export const presentationSettings: readonly SettingDescriptor[] = [
  { id: 'presentation.theme', section: 'Presentation', title: 'Theme', type: 'enum', options: ['dark', 'paper', 'green'], default: 'dark', apply: 'live' },
  { id: 'presentation.scale', section: 'Presentation', title: 'Scaling', type: 'enum', options: ['fit', 'integer'], default: 'fit', apply: 'live' },
  { id: 'presentation.scanlines', section: 'Presentation', title: 'Scanlines', type: 'boolean', default: false, apply: 'live' },
  // Temporary UI control. Removing this descriptor leaves older profiles readable.
  { id: 'presentation.panelLeft', section: 'Presentation', title: 'Show on left', type: 'boolean', default: false, apply: 'live' },
];
