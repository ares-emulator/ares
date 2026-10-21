'use strict';
const fs = require('node:fs');
const path = require('node:path');
const { StdioPeer } = require('../tests/stdio-peer.cjs');
// Ignore documentation and object-key order only. Keep validation, defaults,
// nullability, union branches and unknown-field policy visible in the report.
function normalize(value, key = '') {
  if (Array.isArray(value)) {
    const result = value.map(v => normalize(v));
    return ['required','enum','anyOf','oneOf','type'].includes(key) ? result.sort((a,b) => JSON.stringify(a).localeCompare(JSON.stringify(b))) : result;
  }
  if (value && typeof value === 'object') return Object.fromEntries(Object.entries(value).filter(([k]) => ['properties','$defs','definitions','patternProperties','dependentSchemas'].includes(key) || !['title','description','$schema'].includes(k)).sort(([a],[b]) => a.localeCompare(b)).map(([k,v]) => [k,normalize(v,k)]));
  return value;
}
function signature(schema) {
  const types = s => [...new Set([
    ...(Array.isArray(s.type) ? s.type : s.type ? [s.type] : []),
    ...(s.anyOf || []).flatMap(types),
    ...(s.oneOf || []).flatMap(types),
  ])].sort();
  return normalize({required: schema.required || [], properties: Object.fromEntries(Object.entries(schema.properties || {}).map(([name,s]) => [name,{types:types(s),...('default' in s ? {default:s.default} : {})}]))});
}
function compare(reference, current) {
  const before = new Map(reference.map(t => [t.name,t.inputSchema]));
  const after = new Map(current.map(t => [t.name,t.inputSchema]));
  const missing = [...before.keys()].filter(n => !after.has(n)).sort();
  const added = [...after.keys()].filter(n => !before.has(n)).sort();
  const differences = [];
  for (const name of [...before.keys()].filter(n => after.has(n)).sort()) {
    const a = normalize(before.get(name)), b = normalize(after.get(name));
    const keys = new Set([...Object.keys(a.properties || {}),...Object.keys(b.properties || {})]);
    const fields = [...keys].sort().filter(k => JSON.stringify(a.properties?.[k]) !== JSON.stringify(b.properties?.[k])).map(k => ({field:k,reference:a.properties?.[k] ?? null,current:b.properties?.[k] ?? null}));
    const {properties:ap,...ao} = a, {properties:bp,...bo} = b;
    if (fields.length || JSON.stringify(ao) !== JSON.stringify(bo)) differences.push({name, ...(JSON.stringify(ao) !== JSON.stringify(bo) ? {object:{reference:ao,current:bo}} : {}), fields});
  }
  const signature_differences = [...before.keys()].filter(n => after.has(n)).sort().filter(n => JSON.stringify(signature(before.get(n))) !== JSON.stringify(signature(after.get(n)))).map(name => ({name,reference:signature(before.get(name)),current:signature(after.get(name))}));
  return {signature_matching:before.size-missing.length-signature_differences.length,signature_differences,reference_count:before.size,current_count:after.size,missing,added,matching:before.size-missing.length-differences.length,differences};
}
async function inventory() {
  // tools/list does not read metadata or connect to a native emulator. A fresh
  // impossible connection path prevents accidentally using an active session.
  const peer = new StdioPeer(process.execPath,[path.resolve(__dirname,'../dist/mcp/server.js'),'--connection',path.resolve('/tmp/ares-schema-only-no-session.json'),'--instance','schema-only'],{env:{...process.env,NODE_OPTIONS:''}});
  try { await peer.initialize(); const response = await peer.request('tools/list',{}); if(response.error) throw new Error(JSON.stringify(response.error)); return response.result.tools; }
  finally { await peer.close(); }
}
async function report() {
  const reference = JSON.parse(fs.readFileSync(path.resolve(__dirname,'../tests/reference/fastmcp-input-schemas.json'),'utf8'));
  return {reference:{commit:reference.commit,sha256:reference.sha256,mcp_version:reference.mcp_version},...compare(reference.tools,await inventory())};
}
module.exports = {normalize,signature,compare,inventory,report};
if(require.main === module) report().then(result => process.stdout.write(JSON.stringify(result,null,2)+'\n'),e => {console.error(e); process.exitCode=1;});
