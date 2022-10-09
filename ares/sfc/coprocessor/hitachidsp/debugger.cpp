auto HitachiDSP::Debugger::load(Node::Object parent) -> void {
  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", "HIT");
  tracer.instruction->setAddressBits(23);
  tracer.instruction->setDepth(16);
}

auto HitachiDSP::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(hitachidsp.r.pb << 8 | hitachidsp.r.pc)) {
    tracer.instruction->notify(hitachidsp.disassembleInstruction(), hitachidsp.disassembleContext());
  }
}
