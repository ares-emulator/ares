import { Worker } from 'node:worker_threads';
import { randomUUID, createHash } from 'node:crypto';
import path from 'node:path';
import os from 'node:os';
import { performance } from 'node:perf_hooks';
import { ServiceError, type SessionIdentity, type SessionReply } from './contracts.js';
import type { NativeSessionClient } from './native-client.js';
import type { ScreenshotParams, ScreenshotResult } from './screenshot-contracts.js';
import { publishArtifact } from './artifact-writer.js';

export class Screenshots {
  private jobs = new Map<string, AbortController>();
  private images = new Map<string, { owner: string; identity: SessionIdentity; bytes: Buffer; expires: number }>();
  release(owner: string): void {
    this.jobs.get(owner)?.abort();
    for (const [id, image] of this.images) if (image.owner === owner) this.images.delete(id);
  }
  close(): void { for (const job of this.jobs.values()) job.abort(); this.images.clear(); }
  private expire(): void { for (const [id, image] of this.images) if (performance.now() >= image.expires) this.images.delete(id); }
  read(id: string, offset: number, length: number, owner: string, identity: SessionIdentity) {
    this.expire(); const image = this.images.get(id);
    if (!image) throw new ServiceError('stale_image');
    if (image.owner !== owner) throw new ServiceError('busy');
    if (image.identity.session !== identity.session || image.identity.epoch !== identity.epoch) throw new ServiceError('stale_epoch');
    if (offset >= image.bytes.length || length > image.bytes.length - offset) throw new ServiceError('out_of_bounds');
    return { image_id: id, offset, length, base64: image.bytes.subarray(offset, offset + length).toString('base64') };
  }
  releaseImage(id: string, owner: string) {
    this.expire(); const image = this.images.get(id);
    if (!image) throw new ServiceError('stale_image');
    if (image.owner !== owner) throw new ServiceError('busy');
    this.images.delete(id); return { released: true as const };
  }
  async capture(client: NativeSessionClient, identity: SessionIdentity, p: ScreenshotParams, owner: string, current: () => boolean): Promise<ScreenshotResult> {
    this.expire();
    for (const [id, image] of this.images) if (image.identity.session !== client.identity.session || image.identity.epoch !== client.identity.epoch) this.images.delete(id);
    if (this.jobs.has(owner) || this.jobs.size >= 2 || p.inline && (this.images.size >= 2 || [...this.images.values()].some(image => image.owner === owner))) throw new ServiceError('busy');
    const job = new AbortController(); this.jobs.set(owner, job); const deadline = performance.now() + 25000;
    const guard = () => {
      if (job.signal.aborted || !current()) throw new ServiceError('artifact_cancelled');
      if (client.identity.session !== identity.session || client.identity.epoch !== identity.epoch) throw new ServiceError('stale_epoch');
      if (performance.now() >= deadline) throw new ServiceError('artifact_timeout');
    };
    const unwrap = <T>(reply: SessionReply<T>): T => { if (!reply.ok || !reply.result) throw new ServiceError(reply.error || 'invalid_reply'); return reply.result; };
    let snapshotId: number | undefined;
    try {
      guard(); const snapshot = unwrap(await client.request('video.begin', identity, { source: p.source }, owner)); snapshotId = snapshot.snapshot_id;
      guard(); const pixels = Buffer.alloc(snapshot.bytes);
      for (let offset = 0; offset < pixels.length;) {
        guard();
        const offsets = Array.from({ length: Math.min(4, Math.ceil((pixels.length - offset) / 16384)) }, (_, i) => offset + i * 16384);
        const pages = await Promise.allSettled(offsets.map(async start => unwrap(await client.request('video.read', identity, { snapshotId: snapshot.snapshot_id, offset: start, length: Math.min(16384, pixels.length - start) }, owner))));
        for (let i = 0; i < pages.length; i++) {
          const page = pages[i]; if (page.status === 'rejected') throw page.reason;
          const r = page.value;
          if (r.snapshot_id !== snapshot.snapshot_id || r.offset !== offsets[i] || r.length !== Math.min(16384, pixels.length - offsets[i])) throw new ServiceError('invalid_reply');
          Buffer.from(r.hex, 'hex').copy(pixels, r.offset); offset += r.length;
        }
      }
      guard();
      const encoded = await new Promise<{ png: Buffer; width: number; height: number; region: number[] }>((resolve, reject) => {
        const worker = new Worker(new URL('./png-worker.js', import.meta.url), { workerData: { pixels, sourceWidth: snapshot.width, sourceHeight: snapshot.height, region: p.region, scale: p.scale }, transferList: [pixels.buffer as ArrayBuffer] });
        const abort = () => { void worker.terminate(); reject(new ServiceError('artifact_cancelled')); };
        job.signal.addEventListener('abort', abort, { once: true });
        const done = () => job.signal.removeEventListener('abort', abort);
        worker.once('message', value => { done(); void worker.terminate(); if (value.error) reject(new ServiceError(value.error)); else resolve({ ...value, png: Buffer.from(value.png) }); });
        worker.once('error', () => { done(); reject(new ServiceError('encoding_failed')); });
        worker.once('exit', () => { done(); reject(new ServiceError('encoding_failed')); });
      });
      guard();
      const { snapshot_id, width: source_width, height: source_height, bytes: rawBytes, format, ...native } = snapshot;
      const result: ScreenshotResult = { ...native, source_width, source_height, width: encoded.width, height: encoded.height, region: encoded.region, scale: p.scale, bytes: encoded.png.length, sha256: createHash('sha256').update(encoded.png).digest('hex'), format: 'png' };
      // Cache publication shares the file commit point. A later owner/epoch
      // transition revokes that cache, but cannot turn a saved file into failure.
      const imageId = p.inline ? randomUUID() : undefined;
      const retainInline = () => {
        if (imageId) {
          this.images.set(imageId, { owner, identity: { ...identity }, bytes: encoded.png, expires: performance.now() + 30000 }); result.image_id = imageId;
        }
      };
      if (p.path !== null || !p.inline) {
        const target = p.path ?? path.join(os.tmpdir(), `ares-screenshot-${randomUUID()}.png`);
        const artifact = await publishArtifact(target, p.overwrite, write => write(encoded.png), guard, 8388608, retainInline);
        result.path = artifact.path;
      } else { guard(); retainInline(); }
      return result;
    } finally {
      if (snapshotId !== undefined) await client.request('video.release', identity, { snapshotId }, owner).catch(() => {});
      if (this.jobs.get(owner) === job) this.jobs.delete(owner);
    }
  }
}
