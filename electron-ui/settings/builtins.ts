import type { SettingDescriptor } from './contracts.js';
export const presentationSettings: readonly SettingDescriptor[] = [
  { id: 'presentation.theme', section: 'Presentation', title: 'Theme', type: 'enum', options: ['dark', 'paper', 'green'], default: 'dark', apply: 'live' },
  { id: 'presentation.scale', section: 'Presentation', title: 'Scaling', type: 'enum', options: ['fit', 'integer'], default: 'fit', apply: 'live' },
  { id: 'presentation.scanlines', section: 'Presentation', title: 'Scanlines', type: 'boolean', default: false, apply: 'live' },
  // Temporary UI control. Removing this descriptor leaves older profiles readable.
  { id: 'presentation.panelLeft', section: 'Presentation', title: 'Show on left', type: 'boolean', default: false, apply: 'live' },
];

export const nativeRuntimeSettings: readonly SettingDescriptor[] = [
  { id: 'audio.volume', title: 'Volume', section: 'Audio', type: 'number', min: 0, max: 1, default: 0.4, apply: 'live', nativeKey: 'Audio/Volume' },
  { id: 'audio.mute', title: 'Mute', section: 'Audio', type: 'boolean', default: false, apply: 'live', nativeKey: 'Audio/Mute' },
  { id: 'video.luminance', title: 'Luminance', section: 'Video', type: 'number', min: 0, max: 2, default: 1, apply: 'live', nativeKey: 'Video/Luminance' },
  { id: 'video.saturation', title: 'Saturation', section: 'Video', type: 'number', min: 0, max: 2, default: 1, apply: 'live', nativeKey: 'Video/Saturation' },
  { id: 'video.gamma', title: 'Gamma', section: 'Video', type: 'number', min: 0.1, max: 3, default: 1, apply: 'live', nativeKey: 'Video/Gamma' },
];
