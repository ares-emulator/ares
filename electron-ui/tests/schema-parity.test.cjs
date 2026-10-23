'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const {normalize,compare,report} = require('../scripts/schema-parity.cjs');
test('actual stdio registry matches the recorded FastMCP comparison, including pending differences', async () => {
  const expected = JSON.parse(fs.readFileSync(path.join(__dirname,'reference/schema-parity-report.json'),'utf8'));
  assert.deepEqual(await report(),expected,'Schema drift: inspect and regenerate the report; a recorded difference is not parity approval.');
  assert.deepEqual(expected.missing,[]); assert.deepEqual(expected.added, ['memory_capture_freeze', 'memory_capture_read', 'memory_capture_release', 'memory_capture_status', 'memory_changes', 'memory_compare', 'memory_query_advance', 'memory_query_page', 'memory_query_release', 'memory_query_start', 'memory_record_start', 'memory_record_stop', 'memory_snapshot', 'plugin_logs', 'read_memory_many', 'read_memory_many_va', 'ui_screenshot']);
  assert.equal(expected.reference_count,68);
  assert.equal(expected.signature_matching,68);
  assert.deepEqual(expected.signature_differences,[]);
});
test('schema comparison ignores documentation but preserves validation and tuple order', () => {
  const tool = inputSchema => [{name:'fixture',inputSchema}];
  const schema = {type:'object',properties:{value:{type:'integer',default:3,minimum:0}},required:['value']};
  assert.equal(compare(tool(schema),tool({...schema,title:'Fixture'})).differences.length,0);
  for(const changed of [
    {...schema,additionalProperties:false},
    {...schema,required:[]},
    {...schema,properties:{value:{type:'integer',default:4,minimum:0}}},
    {...schema,properties:{value:{type:'integer',default:3,minimum:1}}},
    {...schema,properties:{value:{type:['integer','null'],default:3}}},
    {...schema,properties:{}},
  ]) assert.equal(compare(tool(schema),tool(changed)).differences.length,1);
  assert.deepEqual(normalize({properties:{title:{type:'string'},description:{type:'integer'}}}),{properties:{description:{type:'integer'},title:{type:'string'}}});
  assert.deepEqual(normalize({required:['b','a']}),normalize({required:['a','b']}));
  assert.notDeepEqual(normalize({prefixItems:[{type:'string'},{type:'integer'}]}),normalize({prefixItems:[{type:'integer'},{type:'string'}]}));
});
