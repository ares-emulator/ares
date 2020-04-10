auto ARMDSP::Debugger::load(Node::Object parent) -> void {
  tracer.instruction = parent->append<Node::Instruction>("Instruction", "ARM");
  tracer.instruction->setAddressBits(32);
}

auto ARMDSP::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(armdsp.pipeline.execute.address)) {
    tracer.instruction->notify(armdsp.disassembleInstruction(), armdsp.disassembleContext());
  }
}
