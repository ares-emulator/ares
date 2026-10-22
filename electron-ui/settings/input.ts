import type { SettingDescriptor, SettingValue, SettingsSnapshot } from './contracts.js';
const names = ['up', 'down', 'left', 'right', 'south', 'east', 'west', 'north', 'select', 'start', 'leftBumper', 'rightBumper', 'leftTrigger', 'rightTrigger', 'leftStick', 'rightStick', ...Array.from({ length: 10 }, (_, i) => `auxiliary${i + 1}`)];
const keyboardDefaults = ['ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight', 'KeyZ', 'KeyX', 'KeyC', 'KeyV', 'ShiftLeft,ShiftRight', 'Enter', 'KeyQ', 'KeyE', '', 'Space', '', '', ...Array.from({ length: 10 }, (_, i) => `Digit${(i + 1) % 10}`)];
const buttonDefaults = [12, 13, 14, 15, 0, 1, 2, 3, 8, 9, 4, 5, 6, 7, 10, 11];
const axes = ['leftX', 'leftY', 'rightX', 'rightY'] as const;
const axisKeys = [['KeyA', 'KeyD'], ['KeyW', 'KeyS'], ['KeyJ', 'KeyL'], ['KeyI', 'KeyK']];
const key = (id: string, title: string, value: string): SettingDescriptor => ({ id, title, section: 'Keyboard (virtual port 1)', type: 'string', control: 'key', default: value, maxLength: 131, apply: 'live' });
const integer = (id: string, title: string, value: number, max: number): SettingDescriptor => ({ id, title, section: 'Gamepad (virtual port 1)', type: 'number', default: value, min: -1, max, integer: true, apply: 'live' });
export const inputDescriptors: readonly SettingDescriptor[] = [
  ...names.map((name, i) => key(`input.keyboard.${name}`, name, keyboardDefaults[i])),
  key('input.keyboard.n64A', 'Nintendo 64 A (overrides south/east)', 'KeyX'),
  key('input.keyboard.n64B', 'Nintendo 64 B (overrides south/east)', 'KeyZ'),
  ...axes.flatMap((axis, i) => [key(`input.keyboard.${axis}.negative`, `${axis} negative`, axisKeys[i][0]), key(`input.keyboard.${axis}.positive`, `${axis} positive`, axisKeys[i][1])]),
  { id: 'input.gamepad.device', title: 'Device (index or first standard pad)', section: 'Gamepad (virtual port 1)', type: 'enum', options: ['disabled', 'first-standard', ...Array.from({ length: 16 }, (_, i) => String(i))], default: 'first-standard', apply: 'live' },
  { id: 'input.gamepad.focus', title: 'Accept gamepad input while focused on', section: 'Gamepad (virtual port 1)', type: 'enum', options: ['app', 'canvas'], default: 'app', apply: 'live' },
  { id: 'input.gamepad.deadzone', title: 'Analog deadzone', section: 'Gamepad (virtual port 1)', type: 'number', default: 0.12, min: 0, max: 0.95, apply: 'live' },
  ...names.map((name, i) => integer(`input.gamepad.${name}`, `${name} button index (-1 disables)`, buttonDefaults[i] ?? -1, 31)),
  ...axes.flatMap((axis, i): SettingDescriptor[] => [
    integer(`input.gamepad.${axis}.axis`, `${axis} axis index (-1 disables)`, i, 15),
    { id: `input.gamepad.${axis}.invert`, title: `${axis} invert`, section: 'Gamepad (virtual port 1)', type: 'boolean', default: false, apply: 'live' },
  ]),
];
export type InputSettings = Record<string, SettingValue>;
export const defaultInputSettings = (): InputSettings => Object.fromEntries(inputDescriptors.map(d => [d.id, d.default]));
export function appliedInputSettings(snapshot: SettingsSnapshot): InputSettings {
  return Object.assign(defaultInputSettings(), Object.fromEntries(snapshot.entries.filter(e => e.descriptor.id.startsWith('input.')).map(e => [e.descriptor.id, e.applied])));
}
export interface GamepadInput { mapping: string; connected?: boolean; buttons: readonly { pressed: boolean }[]; axes: readonly number[] }
export interface VirtualInput { mask: number; x: number; y: number; rx: number; ry: number }
/** Pure mapping logic is shared by the live renderer and fixture checks. */
export function mapInput(settings: InputSettings, pressed: ReadonlySet<string>, system: string,
    gamepads: readonly (GamepadInput | null)[], focused: boolean, canvasFocused: boolean): VirtualInput {
  const result: VirtualInput = { mask: 0, x: 0, y: 0, rx: 0, ry: 0 };
  if (!focused) return result;
  const held = (id: string) => String(settings[id] ?? '').split(',').some(code => pressed.has(code));
  const n64 = system.startsWith('Nintendo 64');
  names.forEach((name, bit) => {
    if (n64 && (bit === 4 || bit === 5)) return;
    if (held(`input.keyboard.${name}`)) result.mask |= 1 << bit;
  });
  if (n64) {
    if (held('input.keyboard.n64A')) result.mask |= 1 << 4;
    if (held('input.keyboard.n64B')) result.mask |= 1 << 6;
  }
  const values = axes.map(axis => (Number(held(`input.keyboard.${axis}.positive`)) - Number(held(`input.keyboard.${axis}.negative`))) * 32767);
  const device = settings['input.gamepad.device'];
  const pad = settings['input.gamepad.focus'] === 'canvas' && !canvasFocused ? undefined
    : device === 'first-standard' ? gamepads.find(pad => pad?.mapping === 'standard' && pad.connected !== false)
    : typeof device === 'string' && /^\d+$/.test(device) ? gamepads[Number(device)] : undefined;
  if (pad && pad.connected !== false) {
    names.forEach((name, bit) => { if (pad.buttons[Number(settings[`input.gamepad.${name}`])]?.pressed) result.mask |= 1 << bit; });
    axes.forEach((axis, i) => {
      const value = pad.axes[Number(settings[`input.gamepad.${axis}.axis`])];
      if (Number.isFinite(value) && Math.abs(value) > Number(settings['input.gamepad.deadzone'])) {
        values[i] = Math.round(Math.max(-1, Math.min(1, value)) * (settings[`input.gamepad.${axis}.invert`] ? -32767 : 32767));
      }
    });
  }
  [result.x, result.y, result.rx, result.ry] = values;
  return result;
}
