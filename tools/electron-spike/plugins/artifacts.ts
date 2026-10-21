import fs from 'node:fs/promises'
import path from 'node:path'
import { createHash, randomUUID } from 'node:crypto'
import { build, transform } from 'esbuild'
import { validateManifest } from './manifest.js'
import type { PluginManifest } from './contracts.js'
export interface PluginArtifact {
  root: string
  hash: string
  manifest: PluginManifest
}
const ignored = new Set(['node_modules', '.git', '.cache', 'artifacts'])
/** Capture inputs before publishing; the renderer never reads a mutable source tree. */
export async function buildArtifact(
  source: string,
  cache: string,
): Promise<PluginArtifact> {
  source = await fs.realpath(source)
  const manifestPath = await fs.realpath(path.join(source, 'manifest.json'))
  if (!manifestPath.startsWith(source + path.sep))
    throw new Error('Plugin manifest escapes its directory')
  const manifestStat = await fs.stat(manifestPath)
  if (!manifestStat.isFile() || manifestStat.size > 16384)
    throw new Error('manifest.json must be under 16 KiB')
  const declared = validateManifest(
    JSON.parse(await fs.readFile(manifestPath, 'utf8')),
  )
  const outputDirectory = path.dirname(declared.entry)
  const ignoredOutput =
    declared.build &&
    outputDirectory !== '.' &&
    !declared.build.entry.startsWith(outputDirectory + '/')
      ? outputDirectory
      : undefined
  const files = new Map<string, Buffer>()
  let total = 0
  async function collect(directory: string, prefix = '') {
    for (const entry of (
      await fs.readdir(directory, { withFileTypes: true })
    ).sort((a, b) => a.name.localeCompare(b.name))) {
      if (
        ignored.has(entry.name) ||
        entry.name.startsWith('.') ||
        (ignoredOutput && prefix + entry.name === ignoredOutput)
      )
        continue
      const relative = prefix + entry.name,
        location = path.join(directory, entry.name)
      if (entry.isDirectory()) {
        await collect(location, relative + '/')
        continue
      }
      const resolved = await fs.realpath(location)
      if (!resolved.startsWith(source + path.sep))
        throw new Error(`Plugin path escapes its directory: ${relative}`)
      const stat = await fs.stat(resolved)
      if (!stat.isFile())
        throw new Error(`Not a regular plugin file: ${relative}`)
      if (stat.size > 2097152)
        throw new Error(`Plugin file exceeds 2 MiB: ${relative}`)
      total += stat.size
      if (total > 16777216 || files.size >= 256)
        throw new Error(
          'Plugin snapshot exceeds 16 MiB / 256 files (exclude dependencies and outputs)',
        )
      files.set(relative, await fs.readFile(resolved))
    }
  }
  await collect(source)
  const bytes = files.get('manifest.json')
  if (!bytes || bytes.length > 16384)
    throw new Error('manifest.json must be under 16 KiB')
  const manifest = validateManifest(JSON.parse(bytes.toString('utf8')))
  const entry = manifest.build?.entry ?? manifest.entry
  if (!files.has(entry))
    throw new Error(`${entry}: build the declared entry before enabling`)
  // Build from the captured tree, while resolving installed dependencies from source.
  const staging = path.join(cache, '.build-' + randomUUID())
  await fs.mkdir(staging, { recursive: true })
  try {
    for (const [relative, content] of files) {
      const file = path.join(staging, relative)
      await fs.mkdir(path.dirname(file), { recursive: true })
      await fs.writeFile(file, content)
    }
    const result = await build({
      absWorkingDir: staging,
      entryPoints: [entry],
      bundle: true,
      platform: 'browser',
      format: 'esm',
      target: 'es2022',
      jsxFactory: manifest.build?.jsxFactory,
      jsxFragment: manifest.build?.jsxFragment,
      outfile: path.join(staging, manifest.entry),
      write: false,
      allowOverwrite: true,
      logLevel: 'silent',
      nodePaths: [path.join(source, 'node_modules')],
      loader: { '.png': 'file', '.webp': 'file', '.woff2': 'file' },
    })
    for (const output of result.outputFiles) {
      const relative = path.relative(staging, output.path)
      if (relative.startsWith('..') || path.isAbsolute(relative))
        throw new Error('Build output escapes artifact')
      files.set(relative, Buffer.from(output.contents))
    }
    const css = manifest.entry.replace(/\.m?js$/, '.css')
    if (
      !manifest.styles &&
      result.outputFiles.some(
        (output) => path.relative(staging, output.path) === css,
      )
    )
      manifest.styles = css
    if (manifest.styles && !files.has(manifest.styles))
      throw new Error(`Missing stylesheet ${manifest.styles}`)
    if (manifest.styles) {
      const checked = await transform(
        files.get(manifest.styles)!.toString('utf8'),
        { loader: 'css', logLevel: 'silent' },
      )
      if (checked.warnings.length)
        throw new Error(
          'Stylesheet validation: ' +
            checked.warnings.map((w) => w.text).join('; '),
        )
      files.set(manifest.styles, Buffer.from(checked.code))
    }
    files.set('manifest.json', Buffer.from(JSON.stringify(manifest)))
    const digest = createHash('sha256')
    total = 0
    for (const [relative, content] of [...files].sort(([a], [b]) =>
      a.localeCompare(b),
    )) {
      total += content.length
      if (content.length > 2097152 || total > 16777216)
        throw new Error('Built plugin exceeds artifact size limit')
      digest.update(JSON.stringify([relative, content.length]))
      digest.update(content)
    }
    const hash = digest.digest('hex'),
      root = path.join(cache, hash)
    try {
      await fs.access(root)
    } catch {
      const publish = path.join(cache, '.publish-' + randomUUID())
      await fs.mkdir(publish)
      try {
        for (const [relative, content] of files) {
          const file = path.join(publish, relative)
          await fs.mkdir(path.dirname(file), { recursive: true })
          await fs.writeFile(file, content, { mode: 0o400 })
        }
        await fs.rename(publish, root)
      } finally {
        await fs.rm(publish, { recursive: true, force: true })
      }
    }
    return { root, hash, manifest }
  } finally {
    await fs.rm(staging, { recursive: true, force: true })
  }
}
