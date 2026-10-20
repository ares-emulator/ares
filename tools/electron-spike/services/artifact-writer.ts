import fs from 'node:fs/promises';
import { linkSync, renameSync } from 'node:fs';
import path from 'node:path';
import { createHash, randomUUID } from 'node:crypto';
import { ServiceError } from './contracts.js';

/** Reference artifact.hpp publication semantics, with asynchronous bounded bulk I/O.
 * No-overwrite uses a hard link, eliminating an exists/rename race. */
export async function publishArtifact(destination: string, overwrite: boolean,
  produce: (write: (data: string | Uint8Array) => Promise<void>) => Promise<void>, guard: () => void,
  maxBytes = 64 * 1024 * 1024, onCommit: () => void = () => {}): Promise<{ path: string; bytes: number; sha256: string }> {
  const target = path.resolve(destination);
  if (target.length > 4096) throw new ServiceError('invalid_path');
  guard();
  await fs.mkdir(path.dirname(target), { recursive: true });
  const temporary = path.join(path.dirname(target), `.ares-artifact-${randomUUID()}.tmp`);
  let handle: Awaited<ReturnType<typeof fs.open>> | undefined;
  let bytes = 0; const hash = createHash('sha256');
  try {
    if (!overwrite) {
      try { await fs.lstat(target); throw new ServiceError('file_exists'); }
      catch (error) { if ((error as NodeJS.ErrnoException).code !== 'ENOENT') throw error; }
    }
    handle = await fs.open(temporary, 'wx', 0o600);
    await produce(async text => {
      guard(); const data = typeof text === 'string' ? Buffer.from(text, 'utf8') : Buffer.from(text.buffer, text.byteOffset, text.byteLength);
      if (bytes + data.length > maxBytes) throw new ServiceError('artifact_too_large');
      await handle!.writeFile(data); bytes += data.length; hash.update(data);
    });
    await handle.sync(); await handle.close(); handle = undefined;
    // The tiny publication step is synchronous so detach/revocation cannot
    // interleave between the final main-thread guard and publication.
    guard();
    if (overwrite) renameSync(temporary, target);
    else linkSync(temporary, target);
    // Non-throwing local cache update; publication is now the completed action.
    onCommit();
    return { path: target, bytes, sha256: hash.digest('hex') };
  } catch (error) {
    if (error instanceof ServiceError) throw error;
    throw new ServiceError((error as NodeJS.ErrnoException).code === 'EEXIST' ? 'file_exists' : 'write_failed');
  } finally {
    await handle?.close().catch(() => {});
    await fs.unlink(temporary).catch(() => {});
  }
}
