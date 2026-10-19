'use strict';
// Development distribution with a bundled runtime; no signing/notarization claim.
const fs = require('node:fs');
const path = require('node:path');
const { execFileSync } = require('node:child_process');
if (process.platform !== 'darwin' || process.arch !== 'arm64') throw new Error('Packaging is currently verified only on macOS arm64');
const root = path.resolve(__dirname, '..');
const destination = path.resolve(process.argv[2] || path.join(root, 'artifacts/distribution/Ares.app'));
if (fs.existsSync(destination)) throw new Error(`Destination already exists: ${destination}`);
const copy = (from, to) => fs.cpSync(from, to, { recursive: true, verbatimSymlinks: true, mode: fs.constants.COPYFILE_FICLONE });
copy(path.join(path.dirname(require.resolve('electron')), 'dist/Electron.app'), destination);
const resources = path.join(destination, 'Contents/Resources'), app = path.join(resources, 'app');
fs.mkdirSync(app);
for (const field of ['CFBundleIdentifier', 'CFBundleName', 'CFBundleDisplayName']) execFileSync('plutil', ['-replace', field, '-string', field === 'CFBundleIdentifier' ? 'org.ares.workbench.spike' : 'Ares Workbench', path.join(destination, 'Contents/Info.plist')]);
for (const file of ['LICENSE', 'LICENSES.chromium.html']) copy(path.join(path.dirname(require.resolve('electron')), 'dist', file), path.join(resources, file));
copy(path.resolve(root, '../../LICENSE'), path.join(resources, 'ARES-LICENSE'));
for (const file of ['main.cjs', 'preload.cjs', 'protocol.cjs', 'renderer.js', 'style.css', 'index.html', 'dist']) copy(path.join(root, file), path.join(app, file));
fs.writeFileSync(path.join(app, 'package.json'), JSON.stringify({ name: 'ares-workbench', version: '0.2.0', main: 'main.cjs' }));
// Ask npm for the installed production tree, then copy it without developer tools.
const production = execFileSync('npm', ['ls', '--omit=dev', '--all', '--parseable'], { cwd: root, encoding: 'utf8' }).trim().split('\n').slice(1);
for (const dependency of production) copy(dependency, path.join(app, path.relative(root, dependency)));
const native = path.join(resources, 'native');
fs.mkdirSync(path.join(native, 'bin'), { recursive: true });
const host = path.join(native, 'bin/ares-electron-host');
copy(process.env.ARES_HOST || path.resolve(root, '../../build-electron-make/bin/ares-electron-host'), host);
copy(path.resolve(root, '../../mia/Database'), path.join(native, 'Database'));
copy(path.resolve(root, '../../mia/Database'), path.join(native, 'bin/Database'));
const loadCommands = execFileSync('otool', ['-l', host], { encoding: 'utf8' });
const rpaths = [...loadCommands.matchAll(/cmd LC_RPATH\n\s+cmdsize \d+\n\s+path (.*?) \(offset/g)].map(match => match[1]);
const libraryRoot = rpaths.find(directory => fs.existsSync(path.join(directory, 'SDL3.framework')));
if (!libraryRoot) throw new Error('Cannot locate native SDL3/MoltenVK runtime libraries');
fs.mkdirSync(path.join(native, 'Frameworks'));
for (const file of ['SDL3.framework', 'libMoltenVK.dylib']) copy(path.join(libraryRoot, file), path.join(native, 'Frameworks', file));
for (const rpath of rpaths) execFileSync('install_name_tool', ['-delete_rpath', rpath, host]);
execFileSync('install_name_tool', ['-add_rpath', '@executable_path/../Frameworks', host]);
execFileSync('codesign', ['--force', '--sign', '-', host]);
for (const file of ['SDL', 'MoltenVK']) copy(path.resolve(libraryRoot, '../licenses', file), path.join(resources, 'licenses', file));
const runtime = path.join(destination, 'Contents/MacOS/Electron');
const nodeVersion = execFileSync(runtime, ['-p', 'process.versions.node'], { encoding: 'utf8', timeout: 5000, env: { ...process.env, ELECTRON_RUN_AS_NODE: '1', NODE_OPTIONS: '', NODE_PATH: '' } }).trim();
if (Number(nodeVersion.split('.')[0]) < 20) throw new Error(`Unsupported bundled Node ${nodeVersion}`);
const launcher = path.join(resources, 'ares-mcp');
fs.writeFileSync(launcher, `#!/bin/sh
ARES_RESOURCES=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
unset NODE_OPTIONS NODE_PATH ELECTRON_ENABLE_LOGGING
export ELECTRON_RUN_AS_NODE=1
exec "$ARES_RESOURCES/../MacOS/Electron" "$ARES_RESOURCES/app/dist/mcp/server.js" "$@"
`, { mode: 0o755 });
fs.writeFileSync(path.join(resources, 'runtime.json'), JSON.stringify({ electron: require('electron/package.json').version, node: nodeVersion, mode: 'ELECTRON_RUN_AS_NODE', platform: process.platform, arch: process.arch }, null, 2));
execFileSync('codesign', ['--force', '--deep', '--sign', '-', destination]);
console.log(JSON.stringify({ app: destination, mcp: launcher, node: nodeVersion }));
