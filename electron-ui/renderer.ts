import { appliedInputSettings, defaultInputSettings, mapInput } from './settings/input.js';
import { blocksGameInput, notify, onSessionAction } from './workspace/shell.js';
import type { FramePacket, RendererStats } from './bridge.js';
import type { SessionIdentity, SessionMethod, SessionReply } from './services/contracts.js';
interface Elements {
  screen: HTMLCanvasElement; viewport: HTMLElement; system: HTMLSelectElement;
  pause: HTMLButtonElement; reset: HTMLButtonElement; open: HTMLButtonElement;
}
const $ = <K extends keyof Elements>(id: K): Elements[K] => {
  const element = document.getElementById(id);
  if (!element) throw new Error(`Missing UI element: ${id}`);
  return element as Elements[K];
};
window.addEventListener('error', event => notify(event.message));
const canvas = $('screen'), context = canvas.getContext('webgl2', { alpha: false, antialias: false, preserveDrawingBuffer: true });
if (!context) throw new Error('WebGL2 is unavailable');
const gl = context;
function shader(type: number, source: string) {
  const s = gl.createShader(type); if (!s) throw new Error('Cannot create shader'); gl.shaderSource(s, source); gl.compileShader(s);
  if (!gl.getShaderParameter(s, gl.COMPILE_STATUS)) throw new Error(gl.getShaderInfoLog(s) || 'Shader compilation failed');
  return s;
}
const program = gl.createProgram();
if (!program) throw new Error('Cannot create WebGL program');
gl.attachShader(program, shader(gl.VERTEX_SHADER, `#version 300 es
out vec2 uv;
void main() {
  vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
  uv = vec2(p.x, 1.0-p.y);
  gl_Position = vec4(p*2.0-1.0, 0.0, 1.0);
}`));
gl.attachShader(program, shader(gl.FRAGMENT_SHADER, `#version 300 es
precision highp float;
in vec2 uv; out vec4 color;
uniform sampler2D frame; uniform float lines; uniform float rows;
void main() {
  color = texture(frame, uv);
  color.rgb *= 1.0 - lines * 0.18 * step(0.5, fract(uv.y * rows));
}`));
gl.linkProgram(program);
if (!gl.getProgramParameter(program, gl.LINK_STATUS)) throw new Error(gl.getProgramInfoLog(program) || 'Program link failed');
gl.useProgram(program);
const texture = gl.createTexture(); gl.bindTexture(gl.TEXTURE_2D, texture);
gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
const lines = gl.getUniformLocation(program, 'lines'), rows = gl.getUniformLocation(program, 'rows');
let system = 'Synthetic', aspect = 4 / 3;
let width = 0, height = 0, pending: FramePacket | null = null, port: MessagePort | undefined, paused = false;
let frames = 0, skipped = 0, last = 0, firstNative = 0, lastNative = 0, firstSequence = 0;
let bridge: number[] = [], upload: number[] = [], inputTimes = new Map<number, number>(), inputAges: number[] = [], latestQueue = 0;
let started = performance.now(), lastStats = started, latestReadback: number[] | null = null;
let receivedFrames = 0, maxPending = 0;
let scaleSetting = 'fit', scanlineSetting = false;
let preferenceRevision = -1;
let inputSettings = defaultInputSettings();
let inputSettingsSignature = JSON.stringify(inputSettings);
function applyPreferences(snapshot: import('./settings/contracts.js').SettingsSnapshot) {
  if (snapshot.revision < preferenceRevision) return;
  preferenceRevision = snapshot.revision;
  const nextInputSettings = appliedInputSettings(snapshot);
  const nextInputSignature = JSON.stringify(nextInputSettings);
  if (nextInputSignature !== inputSettingsSignature) {
    inputSettings = nextInputSettings; inputSettingsSignature = nextInputSignature;
    releaseInput();
  }
  const get = (id: string) => snapshot.entries.find(entry => entry.descriptor.id === id)?.applied;
  document.body.dataset.theme = String(get('presentation.theme') ?? 'dark');
  scaleSetting = String(get('presentation.scale') ?? 'fit');
  scanlineSetting = get('presentation.scanlines') === true;
  if (width) resize();
  if (snapshot.error) notify(snapshot.error);
}
window.ares.preferences.changed(applyPreferences);
void window.ares.preferences.catalog().then(applyPreferences).catch(notify);
function resize() {
  if (!width) return;
  const rect = $('viewport').getBoundingClientRect();
  const displayWidth = height * aspect;
  let factor = Math.min(rect.width / displayWidth, rect.height / height);
  if (scaleSetting === 'integer' && factor >= 1) factor = Math.floor(factor);
  const w = Math.max(1, Math.floor(displayWidth * factor)), h = Math.max(1, Math.floor(height * factor));
  canvas.style.width = w + 'px'; canvas.style.height = h + 'px';
  const backingW = Math.round(w * devicePixelRatio), backingH = Math.round(h * devicePixelRatio);
  if (canvas.width !== backingW || canvas.height !== backingH) { canvas.width = backingW; canvas.height = backingH; }
  gl.viewport(0, 0, canvas.width, canvas.height);
  draw();
}
function draw() {
  gl.uniform1f(lines, scanlineSetting ? 1 : 0); gl.uniform1f(rows, height);
  gl.drawArrays(gl.TRIANGLES, 0, 3);
}
new ResizeObserver(resize).observe($('viewport'));
window.addEventListener('message', event => {
  if (event.source !== window || event.data?.type !== 'ares-frames' || !event.ports[0]) return;
  port?.close(); port = event.ports[0];
  port.onmessage = ({ data }: MessageEvent<import('./bridge.js').FrameMessage>) => {
    if (data.reset) {
      system = data.system;
      document.getElementById('empty-state')!.hidden = system !== 'Synthetic';
      pending = null; frames = skipped = last = firstNative = lastNative = firstSequence = receivedFrames = maxPending = 0;
      bridge = []; upload = []; inputAges = []; inputTimes.clear(); started = performance.now();
      lastMask = -1; syncSession(); window.dispatchEvent(new Event('ares-session-reset')); return;
    }
    receivedFrames++;
    if (pending) throw new Error('Frame credit invariant violated');
    pending = data; maxPending = Math.max(maxPending, 1);
  };
  port.start();
});
// Bits represent ares's virtual pad, so existing per-system mappings remain authoritative.
const pressed = new Set<string>();
let inputSeq = 0, lastMask = -1, lastAxes = '';
function sendInput(mask: number, x = 0, y = 0, rx = 0, ry = 0) {
  const axes = `${x},${y},${rx},${ry}`;
  if (mask === lastMask && axes === lastAxes) return;
  lastMask = mask; lastAxes = axes; inputSeq++;
  if (frames > 60) inputTimes.set(inputSeq, performance.now());
  if (inputTimes.size > 128) inputTimes.delete(inputTimes.keys().next().value!);
  window.ares.command({ type: 'input', sequence: inputSeq, mask, x, y, rx, ry });
}
function pollInput() {
  const input = mapInput(inputSettings, pressed, system, [...navigator.getGamepads()], document.hasFocus() && !blocksGameInput(), document.activeElement === canvas);
  sendInput(input.mask, input.x, input.y, input.rx, input.ry);
}
function nativeKey(code: string) {
  if (/^Key[A-Z]$/.test(code)) return code.slice(3);
  if (/^Digit[0-9]$/.test(code)) return 'Num' + code.slice(5);
  if (/^Numpad[0-9]$/.test(code)) return 'Keypad' + code.slice(6);
  const names: Record<string, string> = { ArrowUp: 'Up', ArrowDown: 'Down', ArrowLeft: 'Left', ArrowRight: 'Right',
    ShiftLeft: 'LeftShift', ShiftRight: 'RightShift', ControlLeft: 'LeftControl', ControlRight: 'RightControl',
    AltLeft: 'LeftAlt', AltRight: 'RightAlt', MetaLeft: 'LeftSuper', MetaRight: 'RightSuper',
    Backquote: 'Tilde', Minus: 'Dash', Equal: 'Equal', BracketLeft: 'LeftBracket', BracketRight: 'RightBracket',
    Backslash: 'Backslash', Semicolon: 'Semicolon', Quote: 'Apostrophe', Comma: 'Comma', Period: 'Period',
    Slash: 'Slash', NumpadEnter: 'Enter', NumpadAdd: 'Add', NumpadSubtract: 'Subtract',
    NumpadMultiply: 'Multiply', NumpadDivide: 'Divide', NumpadDecimal: 'Point',
    PageUp: 'Pageup', PageDown: 'Pagedown' };
  return names[code] || code;
}
window.addEventListener('keydown', event => {
  if (document.activeElement !== canvas) return;
  event.preventDefault(); pressed.add(event.code); pollInput();
  if (!event.repeat) window.ares.command({ type: 'key', name: nativeKey(event.code), value: true });
});
window.addEventListener('keyup', event => {
  pressed.delete(event.code); pollInput();
  window.ares.command({ type: 'key', name: nativeKey(event.code), value: false });
});
function releaseInput() { pressed.clear(); sendInput(0); window.ares.command({ type: 'clear' }); }
window.addEventListener('ares-input-release', releaseInput);
window.addEventListener('blur', releaseInput); canvas.addEventListener('blur', releaseInput);
function mouseInput(event: MouseEvent) {
  if (document.activeElement !== canvas) return;
  const rect = canvas.getBoundingClientRect();
  // DOM and HID use different middle/right button bit ordering.
  const mask = (event.buttons & 1) | ((event.buttons & 4) >> 1) | ((event.buttons & 2) << 1);
  window.ares.command({ type: 'mouse', x: event.type === 'mousemove' ? event.movementX * width / rect.width : 0,
    y: event.type === 'mousemove' ? event.movementY * height / rect.height : 0, mask });
}
canvas.addEventListener('mousedown', event => { canvas.focus(); mouseInput(event); });
canvas.addEventListener('mousemove', mouseInput); canvas.addEventListener('mouseup', mouseInput);
canvas.addEventListener('contextmenu', event => event.preventDefault());
canvas.onclick = () => canvas.focus();
$('open').onclick = () => window.ares.open({ system: $('system').value });
window.ares.systems().then(systems => {
  for (const name of systems) { const option = document.createElement('option'); option.value = option.textContent = name; $('system').append(option); }
}).catch(error => { notify(error); });
let sessionIdentity: SessionIdentity | undefined;
function acceptSessionState(reply: SessionReply) {
  if (!reply.ok || !reply.result) return;
  const previous = sessionIdentity;
  sessionIdentity = { session: reply.session, epoch: reply.epoch };
  paused = reply.result.paused && reply.result.operation_id == null;
  $('pause').textContent = paused ? 'Resume' : 'Pause';
  if (previous && (previous.session !== reply.session || previous.epoch !== reply.epoch)) window.dispatchEvent(new Event('ares-session-reset'));
}
window.ares.onSessionState(acceptSessionState);
async function sessionRequest(method: SessionMethod) {
  let identity = sessionIdentity;
  if (!identity && method !== 'session.status') {
    const status = await window.ares.session('session.status');
    if (!status.ok) throw new Error(status.error);
    acceptSessionState(status);
    identity = { session: status.session, epoch: status.epoch };
  }
  // Capture the identity for this action. Overlapping reload/status synchronization
  // may clear the shared field; stale actions still fail instead of being replayed.
  const reply = await window.ares.session(method, identity);
  if (!reply.ok) throw new Error(reply.error);
  acceptSessionState(reply);
  return reply;
}
async function syncSession() {
  sessionIdentity = undefined;
  try { await sessionRequest('session.status'); } catch (error) { notify(error); }
}
$('pause').onclick = async () => {
  $('pause').disabled = true;
  try { await sessionRequest(paused ? 'session.resume' : 'session.pause'); }
  catch (error) { notify(error); await syncSession(); }
  finally { $('pause').disabled = false; }
};
$('reset').onclick = async () => {
  try { await sessionRequest('session.reset'); }
  catch (error) { notify(error); await syncSession(); }
};
onSessionAction(action => {
  if (action === 'open') $('open').click();
  else if (action === 'pause') $('pause').click();
  else $('reset').click();
});
function stress() { const until = performance.now() + 250; while (performance.now() < until) {} }
window.ares.onStatus(text => { if (!/^ready:|^stopped$/.test(text)) notify(text); });
const percentile = (array: number[], p: number) => array.length ? [...array].sort((a, b) => a - b)[Math.min(array.length - 1, Math.floor(array.length * p))] : 0;
function record(array: number[], value: number) { array.push(value); if (array.length > 3600) array.shift(); }
function stats(): RendererStats {
  return { system, frames, receivedFrames, maxPending, skipped, width, height,
    seconds: (performance.now() - started) / 1000,
    nativeFPS: lastNative > firstNative ? (last - firstSequence) * 1000 / (lastNative - firstNative) : 0,
    bridgeToSubmitP50Ms: percentile(bridge, .5), bridgeToSubmitP95Ms: percentile(bridge, .95),
    uploadSubmitP95Ms: percentile(upload, .95), inputToFrameSubmitP95Ms: percentile(inputAges, .95),
    nativeQueueMs: latestQueue, glError: gl.getError(), readback: latestReadback };
}
function tick() {
  pollInput();
  if (pending) {
    const frame = pending; pending = null;
    const changedAspect = aspect !== frame.aspect; aspect = frame.aspect;
    const before = performance.now();
    if (frame.width !== width || frame.height !== height) {
      width = frame.width; height = frame.height;
      gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, width, height, 0, gl.RGBA, gl.UNSIGNED_BYTE, frame.pixels);
      resize();
    } else gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, width, height, gl.RGBA, gl.UNSIGNED_BYTE, frame.pixels);
    if (changedAspect) resize();
    draw();
    const after = performance.now();
    record(upload, after - before);
    record(bridge, performance.timeOrigin + after - frame.receivedMs);
    latestQueue = frame.queueMs;
    if (inputTimes.has(frame.inputSequence)) {
      record(inputAges, after - inputTimes.get(frame.inputSequence)!);
      for (const seq of inputTimes.keys()) if (seq <= frame.inputSequence) inputTimes.delete(seq);
    }
    if (last) skipped += Math.max(0, frame.sequence - last - 1);
    else { firstNative = frame.nativeMs; firstSequence = frame.sequence; }
    last = frame.sequence; lastNative = frame.nativeMs; frames++;
    port!.postMessage({ ack: frame.sequence, generation: frame.generation });
  }
  requestAnimationFrame(tick);
}
requestAnimationFrame(tick);
window.stats = stats;
window.sendInput = sendInput;
window.ares.ready();
const params = new URLSearchParams(location.search);
if (params.get('autotest') === '1') {
  setTimeout(stress, 2500);
  setTimeout(() => {
    const pixel = new Uint8Array(4); gl.readPixels(0, 0, 1, 1, gl.RGBA, gl.UNSIGNED_BYTE, pixel);
    latestReadback = [...pixel];
    window.ares.command({ type: 'report', report: stats() });
  }, Math.max(5, Math.min(3600, Number(params.get('duration')) || 12)) * 1000);
}
