auto ARM7TDMI::serialize(serializer& s) -> void {
  s(processor);
  s(pipeline);
  s(carry);
  s(irq);
}

auto ARM7TDMI::Processor::serialize(serializer& s) -> void {
  s(r0.data);
  s(r1.data);
  s(r2.data);
  s(r3.data);
  s(r4.data);
  s(r5.data);
  s(r6.data);
  s(r7.data);
  s(r8.data);
  s(r9.data);
  s(r10.data);
  s(r11.data);
  s(r12.data);
  s(r13.data);
  s(r14.data);
  s(r15.data);
  s(cpsr);
  s(fiq.r8.data);
  s(fiq.r9.data);
  s(fiq.r10.data);
  s(fiq.r11.data);
  s(fiq.r12.data);
  s(fiq.r13.data);
  s(fiq.r14.data);
  s(fiq.spsr);
  s(irq.r13.data);
  s(irq.r14.data);
  s(irq.spsr);
  s(svc.r13.data);
  s(svc.r14.data);
  s(svc.spsr);
  s(abt.r13.data);
  s(abt.r14.data);
  s(abt.spsr);
  s(und.r13.data);
  s(und.r14.data);
  s(und.spsr);
}

auto ARM7TDMI::PSR::serialize(serializer& s) -> void {
  s(m);
  s(t);
  s(f);
  s(i);
  s(v);
  s(c);
  s(z);
  s(n);
}

auto ARM7TDMI::Pipeline::serialize(serializer& s) -> void {
  s(reload);
  s(nonsequential);
  s(fetch.address);
  s(fetch.instruction);
  s(fetch.thumb);
  s(decode.address);
  s(decode.instruction);
  s(decode.thumb);
  s(execute.address);
  s(execute.instruction);
  s(execute.thumb);
}
