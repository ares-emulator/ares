auto ARMv6M::PSR::serialize(serializer& s) -> void {
  s(n);
  s(z);
  s(c);
  s(v);
  s(it);
}

auto ARMv6M::Processor::serialize(serializer& s) -> void {
  for(auto& value : r) s(value);
  psr.serialize(s);
}

auto ARMv6M::serialize(serializer& s) -> void {
  processor.serialize(s);
  s(opcode);
  s(instructionAddress);
}
