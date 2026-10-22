'use strict'
// Ship the complete browser SDK declaration graph with scaffolded plugins.
const fs = require('node:fs')
const path = require('node:path')
const root = path.resolve(__dirname, '..')
const source = path.join(root, 'dist/browser')
const destination = path.join(root, 'templates/plugin-tsx/src/sdk')
fs.rmSync(destination, { recursive: true, force: true })
const visited = new Set()
function copy(relative) {
  if (visited.has(relative)) return
  visited.add(relative)
  const file = path.resolve(source, relative)
  if (!file.startsWith(source + path.sep)) throw new Error('SDK declaration escaped browser output')
  const text = fs.readFileSync(file, 'utf8')
  const output = path.join(destination, relative)
  fs.mkdirSync(path.dirname(output), { recursive: true })
  fs.writeFileSync(output, text)
  for (const match of text.matchAll(/(?:from\s*|import\s*\()\s*['"]([^'"]+)['"]/g)) {
    if (!match[1].startsWith('.')) throw new Error(`SDK must not depend on runtime packages: ${match[1]}`)
    copy(path.normalize(path.join(path.dirname(relative), match[1].replace(/\.js$/, '.d.ts'))))
  }
}
copy('plugins/sdk.d.ts')
