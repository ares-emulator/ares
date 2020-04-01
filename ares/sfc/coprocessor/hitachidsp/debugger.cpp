auto HitachiDSP::Debugger::load(Node::Object parent, Node::Object from) -> void {
  tracer.instruction = Node::append<Node::Instruction>(parent, from, "Instruction", "HIT");
  tracer.instruction->setAddressBits(23);
}

auto HitachiDSP::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(hitachidsp.r.pb << 8 | hitachidsp.r.pc)) {
    tracer.instruction->notify(hitachidsp.disassembleInstruction(), hitachidsp.disassembleContext());
  }
}
