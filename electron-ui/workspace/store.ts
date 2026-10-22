import fs from 'node:fs';
import path from 'node:path';
import { randomUUID } from 'node:crypto';
import { defaultLayout, validateLayout, type WorkspaceLayout, type WorkspaceSnapshot } from './model.js';
/** Profile ownership is acquired before this store opens. Invalid provisioning is never overwritten. */
export class WorkspaceStore {
  private layout = defaultLayout();
  private error?: string;
  constructor(readonly filename: string) {
    try {
      if (fs.statSync(filename).size > 65536) throw new Error('workspace_file_too_large');
      this.layout = validateLayout(JSON.parse(fs.readFileSync(filename, 'utf8')));
    } catch (error) {
      if ((error as NodeJS.ErrnoException).code !== 'ENOENT') this.error = `Cannot read workspace: ${String(error)}. Repair ${filename} and restart.`;
    }
  }
  load(): WorkspaceSnapshot { return { layout: structuredClone(this.layout), ...(this.error ? { error: this.error } : {}) }; }
  save(value: WorkspaceLayout): WorkspaceSnapshot {
    const layout = validateLayout(value);
    if (this.error) throw new Error(this.error);
    const text = JSON.stringify(layout, null, 2) + '\n';
    if (Buffer.byteLength(text) > 65536) throw new Error('workspace_file_too_large');
    fs.mkdirSync(path.dirname(this.filename), { recursive: true });
    const temporary = `${this.filename}.${randomUUID()}.tmp`;
    try { fs.writeFileSync(temporary, text, { flag: 'wx', mode: 0o600 }); fs.renameSync(temporary, this.filename); }
    finally { fs.rmSync(temporary, { force: true }); }
    this.layout = layout; return this.load();
  }
}
