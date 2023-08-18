auto CPU::Debugger::load(Node::Object parent) -> void {
  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", "CPU");
  tracer.instruction->setAddressBits(16);
}

auto CPU::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(cpu.PC)) {
    tracer.instruction->notify(cpu.disassembleInstruction(), {cpu.disassembleContext(), " [c:", cpu.io.scanlineCycles, " l:", tia.io.vcounter, " p:", tia.io.hcounter, "]"});
  }
}


