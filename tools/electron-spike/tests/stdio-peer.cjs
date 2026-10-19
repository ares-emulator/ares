'use strict';
const { spawn } = require('node:child_process');
const { once } = require('node:events');
class StdioPeer {
  constructor(command, args, options = {}) {
    this.child = spawn(command, args, { stdio: ['pipe', 'pipe', 'pipe'], ...options });
    this.closed = once(this.child, 'close'); this.pending = new Map(); this.id = 0; this.stderr = ''; this.messages = [];
    let buffer = '';
    this.child.stderr.on('data', b => this.stderr += b);
    this.child.stdin.on('error', () => {});
    this.child.stdout.on('data', chunk => {
      buffer += chunk; let end;
      while ((end = buffer.indexOf('\n')) >= 0) {
        const line = buffer.slice(0, end); buffer = buffer.slice(end + 1);
        let message;
        try { message = JSON.parse(line); } catch { this.fail(new Error(`Non-MCP stdout: ${line}`)); return; }
        if (message.jsonrpc !== '2.0') { this.fail(new Error(`Invalid MCP envelope: ${line}`)); return; }
        this.messages.push(message);
        const pending = this.pending.get(message.id);
        if (pending) { clearTimeout(pending.timer); this.pending.delete(message.id); pending.resolve(message); }
      }
    });
    this.child.once('error', error => this.fail(error));
    this.child.once('close', () => this.fail(new Error(`MCP closed: ${this.stderr}`)));
  }
  fail(error) { for (const p of this.pending.values()) { clearTimeout(p.timer); p.reject(error); } this.pending.clear(); }
  request(method, params, timeoutMs = 8000) {
    const id = ++this.id;
    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => { this.pending.delete(id); reject(new Error(`MCP timeout ${method}: ${this.stderr}`)); }, timeoutMs);
      this.pending.set(id, { resolve, reject, timer }); this.child.stdin.write(JSON.stringify({ jsonrpc: '2.0', id, method, params }) + '\n');
    });
  }
  async initialize() {
    const response = await this.request('initialize', { protocolVersion: '2025-06-18', capabilities: {}, clientInfo: { name: 'ares-live-test', version: '1' } }, 30000);
    if (response.error) throw new Error(JSON.stringify(response.error));
    this.child.stdin.write(JSON.stringify({ jsonrpc: '2.0', method: 'notifications/initialized' }) + '\n'); return response;
  }
  async tool(name, args = {}) {
    const response = await this.request('tools/call', { name, arguments: args });
    if (response.error) return { error: response.error, isError: true };
    return { ...JSON.parse(response.result.content[0].text), isError: response.result.isError === true };
  }
  async close() {
    this.child.stdin.end();
    const timer = setTimeout(() => this.child.kill('SIGKILL'), 2000);
    try { return await this.closed; } finally { clearTimeout(timer); }
  }
}
module.exports = { StdioPeer };
