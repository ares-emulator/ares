import fs from 'node:fs'
import path from 'node:path'

export function profileRoot(appData: string, name = 'default'): string {
  if (!/^[a-zA-Z0-9][a-zA-Z0-9_-]{0,63}$/.test(name))
    throw new Error('invalid_profile_name: use 1–64 letters, digits, underscores or hyphens')
  return path.join(appData, 'Ares Workbench', 'profiles', name)
}

// Explicit save overrides may select a subtree, but cannot share writable data
// across independently locked profiles. Resolve symlinks before accepting it.
export function profileSaves(root: string, override?: string): string {
  const canonicalRoot = fs.realpathSync(root)
  const directory = path.resolve(override || path.join(canonicalRoot, 'saves'))
  const contained = (candidate: string) => {
    const relative = path.relative(canonicalRoot, candidate)
    return relative !== '' && relative !== '..' && !relative.startsWith(`..${path.sep}`) && !path.isAbsolute(relative)
  }
  // Check existing ancestors before mkdir so a symlink cannot redirect creation.
  let ancestor = directory
  while (!fs.existsSync(ancestor)) ancestor = path.dirname(ancestor)
  const resolved = path.join(fs.realpathSync(ancestor), path.relative(ancestor, directory))
  if (!contained(resolved)) throw new Error('save_directory_must_be_inside_profile')
  fs.mkdirSync(directory, { recursive: true })
  return fs.realpathSync(directory)
}
