import fs from 'node:fs/promises'
import path from 'node:path'
import { fileURLToPath } from 'node:url'
import { LocalServiceClient } from '../services/local-endpoint.js'
import { buildArtifact } from './artifacts.js'
import { validateManifest } from './manifest.js'
import type { PluginInfo } from './contracts.js'
const args = process.argv.slice(2),
  operation = args.shift(),
  values: string[] = [],
  options: Record<string, string> = {}
for (let i = 0; i < args.length; i++) {
  if (args[i] === '--disabled') options.disabled = 'true'
  else if (args[i].startsWith('--')) {
    if (!args[i + 1] || args[i + 1].startsWith('--'))
      throw new Error(`Missing value for ${args[i]}`)
    options[args[i].slice(2)] = args[++i]
  } else values.push(args[i])
}
async function main() {
  if (operation === 'scaffold') {
    if (!values[0]) throw new Error('scaffold requires a new directory')
    const directory = path.resolve(values[0]),
      template = fileURLToPath(
        new URL('../../templates/plugin-tsx/', import.meta.url),
      )
    const manifest = validateManifest({
      ...JSON.parse(
        await fs.readFile(path.join(template, 'manifest.json'), 'utf8'),
      ),
      id: options.id ?? 'local.tool',
      name: options.name ?? 'Local tool',
    })
    await fs.mkdir(directory)
    await fs.cp(template, directory, { recursive: true })
    await fs.writeFile(
      path.join(directory, 'manifest.json'),
      JSON.stringify(manifest, null, 2) + '\n',
    )
    return { directory, manifest }
  }
  if (operation === 'build') {
    if (!values[0]) throw new Error('build requires a plugin directory')
    const source = path.resolve(values[0]),
      built = await buildArtifact(
        source,
        path.resolve(options.output ?? path.join(source, '.ares-plugin-build')),
      )
    return {
      artifact: built.hash,
      directory: built.root,
      manifest: built.manifest,
    }
  }
  if (!options.connection || !options.instance)
    throw new Error(
      'Use --connection <file> --instance <id> to select the app explicitly',
    )
  const client = new LocalServiceClient(
    path.resolve(options.connection),
    options.instance,
  )
  async function list(): Promise<PluginInfo[]> {
    const all: PluginInfo[] = []
    let offset = 0
    let limit = Number(options['page-size'] ?? '4')
    if (!Number.isInteger(limit) || limit < 1 || limit > 4)
      throw new Error('--page-size must be 1–4')
    for (;;) {
      let page
      try {
        page = await client.manage({ operation: 'list', offset, limit })
      } catch (error) {
        if (limit > 1 && String(error).includes('plugin_result_too_large')) {
          limit = 1
          continue
        }
        throw error
      }
      all.push(...page.plugins)
      if (page.nextOffset === null) return all
      offset = page.nextOffset
    }
  }
  try {
    if (operation === 'list') return { plugins: await list() }
    if (operation === 'reset-startup')
      return await client.manage({ operation: 'resetStartup' })
    if (operation === 'add') {
      if (!values[0]) throw new Error('add requires a directory')
      return await client.manage({
        operation: 'add',
        source: path.resolve(values[0]),
      })
    }
    const id = values[0]
    if (!id) throw new Error('A plugin id is required')
    if (operation === 'enable' || operation === 'disable')
      return await client.manage({
        operation: 'enable',
        id,
        enabled: operation !== 'disable' && !options.disabled,
      })
    const plugin = (await list()).find((p) => p.manifest?.id === id)
    if (!plugin) throw new Error('Unknown plugin')
    if (operation === 'reload') {
      if (!plugin.enabled) throw new Error('Enable the plugin before reloading')
      await client.manage({ operation: 'reload', id })
      if (!plugin.lease)
        return { plugin: (await list()).find((p) => p.manifest?.id === id) }
      for (let i = 0; i < 150; i++) {
        const next = (await list()).find((p) => p.manifest?.id === id)
        if (next?.error && next.reload === 'error') throw new Error(next.error)
        if (
          next?.phase === 'active' &&
          next.lease?.token !== plugin.lease.token
        )
          return { plugin: next }
        await new Promise((resolve) => setTimeout(resolve, 50))
      }
      throw new Error('Reload did not finish; command was not replayed')
    }
    if (operation === 'open' || operation === 'command') {
      const token = options.token ?? plugin.lease?.token
      if (!token) throw new Error('Plugin is not active for this game')
      const name =
        values[1] ??
        (operation === 'open'
          ? plugin.manifest?.contributes.panels?.[0]?.id
          : undefined)
      if (!name) throw new Error('A panel or command id is required')
      return await client.manage({ operation, id, token, name })
    }
    throw new Error(
      'Commands: scaffold, build, list, add, enable [--disabled], reload, open, command',
    )
  } finally {
    client.close()
  }
}
main().then(
  (result) => process.stdout.write(JSON.stringify(result, null, 2) + '\n'),
  (error) => {
    process.stderr.write(String(error) + '\n')
    process.exitCode = 1
  },
)
