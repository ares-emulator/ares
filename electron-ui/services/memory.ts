import { MAX_TRANSFER, ServiceError, type SessionClient, type SessionIdentity, type SessionReply, type MemoryRegion, type MemoryRead, type MemoryWritten } from './contracts.js';
export type Endian = 'little' | 'big';
export type IntegerWidth = 1 | 2 | 4;
export interface RegionHandle extends MemoryRegion { identity: SessionIdentity }
function unwrap<T>(reply: SessionReply<T>): T {
  if (!reply.ok || !reply.result) throw new ServiceError(reply.error || 'invalid_reply');
  return reply.result;
}
export function hexToBytes(hex: string): Uint8Array {
  if (!/^(?:[0-9a-fA-F]{2})*$/.test(hex)) throw new ServiceError('invalid_hex');
  return Uint8Array.from(hex.match(/../g) || [], byte => parseInt(byte, 16));
}
export function bytesToHex(bytes: Uint8Array): string { return Array.from(bytes, byte => byte.toString(16).padStart(2, '0')).join(''); }
function format(width: IntegerWidth, endian: Endian): void {
  if (![1, 2, 4].includes(width) || !['little', 'big'].includes(endian)) throw new ServiceError('invalid_integer_format');
}
export function decodeUnsigned(bytes: Uint8Array, width: IntegerWidth, endian: Endian, offset = 0): number {
  format(width, endian);
  if (!Number.isSafeInteger(offset) || offset < 0 || offset + width > bytes.byteLength) throw new ServiceError('out_of_bounds');
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  return width === 1 ? view.getUint8(offset) : width === 2 ? view.getUint16(offset, endian === 'little') : view.getUint32(offset, endian === 'little');
}
export function encodeUnsigned(value: number, width: IntegerWidth, endian: Endian): Uint8Array {
  format(width, endian);
  if (!Number.isSafeInteger(value) || value < 0 || value >= 2 ** (8 * width)) throw new ServiceError('integer_out_of_range');
  const bytes = new Uint8Array(width), view = new DataView(bytes.buffer);
  if (width === 1) view.setUint8(0, value);
  else if (width === 2) view.setUint16(0, value, endian === 'little');
  else view.setUint32(0, value, endian === 'little');
  return bytes;
}
/** Region handles retain the enumeration identity. Stale work never binds itself to a new game. */
export class MemoryClient {
  constructor(private client: SessionClient) {}
  async regions(identity?: SessionIdentity): Promise<RegionHandle[]> {
    const reply = await this.client.request('memory.regions', identity);
    return unwrap(reply).regions.map(region => ({ ...region, identity: { session: reply.session, epoch: reply.epoch } }));
  }
  async read(region: RegionHandle, offset: number, length: number): Promise<MemoryRead> {
    return unwrap(await this.client.request('memory.read', region.identity, { region: region.id, offset, length }));
  }
  async readMany(reads: { region: RegionHandle; offset: number; length: number }[]) {
    if (!reads.length) throw new ServiceError('invalid_request');
    const identity = reads[0]!.region.identity;
    if (reads.some(r => r.region.identity.session !== identity.session || r.region.identity.epoch !== identity.epoch)) throw new ServiceError('stale_region');
    return unwrap(await this.client.request('memory.readMany', identity, { reads: reads.map(r => ({ region: r.region.id, offset: r.offset, length: r.length })) }));
  }
  async write(region: RegionHandle, offset: number, bytes: Uint8Array): Promise<MemoryWritten> {
    if (!bytes.byteLength || bytes.byteLength > MAX_TRANSFER) throw new ServiceError('invalid_request');
    return unwrap(await this.client.request('memory.write', region.identity, { region: region.id, offset, hex: bytesToHex(bytes) }));
  }
  async readUnsigned(region: RegionHandle, offset: number, width: IntegerWidth, endian: Endian): Promise<number> {
    format(width, endian);
    return decodeUnsigned(hexToBytes((await this.read(region, offset, width)).hex), width, endian);
  }
  writeUnsigned(region: RegionHandle, offset: number, value: number, width: IntegerWidth, endian: Endian): Promise<MemoryWritten> {
    return this.write(region, offset, encodeUnsigned(value, width, endian));
  }
}
