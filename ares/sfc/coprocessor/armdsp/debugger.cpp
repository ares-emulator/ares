auto ARMDSP::Debugger::load(Node::Object parent, Node::Object from) -> void {
  tracer.instruction = Node::append<Node::Instruction>(parent, from, "Instruction", "ARM");
  tracer.instruction->setAddressBits(32);
}

auto ARMDSP::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(armdsp.pipeline.execute.address)) {
    tracer.instruction->notify(armdsp.disassembleInstruction(), armdsp.disassembleContext());
  }
}
