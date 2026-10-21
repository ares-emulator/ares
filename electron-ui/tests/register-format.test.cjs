'use strict';
const test=require('node:test');const assert=require('node:assert/strict');
const {formatRegisters,formatOperation}=require('../dist/mcp/n64-tools.js');
const registers=()=>({pc:'0x80001000',next_pc:'0x80001004',delay_slot:false,hi:'0x0',lo:'0x0',gpr:Array(32).fill('0x0'),cop0:Array(32).fill('0x0'),fpr:Array(32).fill('0x0'),fcr0:'0x0',fcr31:'0x1800084'});
test('reference-shaped compact/verbose registers classify architectural FR views without JSON NaN',()=>{
  const raw=registers();raw.cop0[1]=null;raw.fpr[0]='0x800000007f800000';
  const compact=formatRegisters(raw);assert.equal(compact.gpr,undefined);assert.equal(compact.gpr_named.zero,'0x0');assert.equal(compact.fpr,undefined);assert.equal(compact.cop0,undefined);
  const paired=formatRegisters(raw,true,true);assert.equal(paired.fpr[0].f32_class,'+inf');assert.equal(paired.fpr[0].f32,null);assert.equal(paired.fpr[1].f32_class,'-zero');assert.equal(paired.fpr[1].dword,raw.fpr[0]);assert.equal(paired.cop0.Random,null);
  raw.cop0[12]='0x04000000';raw.fpr[0]='0x7fc00000';raw.fpr[1]='0x7ff0000000000000';raw.fpr[2]='0x8000000000000000';raw.fpr[3]='0x1';
  const full=formatRegisters(raw,false,false,true);assert.equal(full.gpr.length,32);assert.equal(full.fpr[0].f32_class,'nan');assert.equal(full.fpr[1].f64_class,'+inf');assert.equal(full.fpr[2].f64_class,'-zero');assert.equal(full.fpr[3].f64_class,'subnormal');assert.equal(full.status.FR,1);assert.equal(full.fcr31.flags.I,1);assert.equal(full.fcr31.C,1);
  assert.doesNotThrow(()=>JSON.parse(JSON.stringify(full)));
});
test('completion formatting uses the frozen snapshot and honors optional groups',()=>{
  const raw=registers();const operation={completion_registers:raw,debug_stop:{valid:false},state:'done'};
  const result=formatOperation(operation,true,true);raw.gpr[4]='0xffff';
  assert.equal(result.completion_registers.gpr_named.a0,'0x0');assert.equal(result.completion_registers.fpr.length,32);assert.equal(result.completion_registers.cop0.Random,'0x0');assert.equal(formatOperation(operation).completion_registers.fpr,undefined);
});
