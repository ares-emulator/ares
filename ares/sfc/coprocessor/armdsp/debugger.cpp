auto ARMDSP::Debugger::load(Node::Object parent) -> void {
  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", "ARM");
  tracer.instruction->setAddressBits(32);
  tracer.instruction->setDepth(16);
}

auto ARMDSP::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(armdsp.pipeline.execute.address)) {
    tracer.instruction->notify(armdsp.disassembleInstruction(), armdsp.disassembleContext());
  }
}
