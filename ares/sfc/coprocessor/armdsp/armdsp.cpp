ArmDSP armdsp;
#include "memory.cpp"
#include "io.cpp"
#include "serialization.cpp"

auto ArmDSP::load(Node::Object parent, Node::Object from) -> void {
  node = Node::append<Node::Component>(parent, from, "ARM");
  from = Node::scan(parent = node, from);

  eventInstruction = Node::append<Node::Instruction>(parent, from, "Instruction", "ARM");
  eventInstruction->setAddressBits(32);
}

auto ArmDSP::unload() -> void {
  eventInstruction = {};
  node = {};

  cpu.coprocessors.removeByValue(this);
  Thread::destroy();
}

auto ArmDSP::boot() -> void {
  //reset hold delay
  while(bridge.reset) {
    step(1);
    continue;
  }

  //reset sequence delay
  if(bridge.ready == false) {
    step(65'536);
    bridge.ready = true;
  }
}

auto ArmDSP::main() -> void {
  processor.cpsr.t = 0;  //force ARM mode
  if(eventInstruction->enabled() && eventInstruction->address(pipeline.execute.address)) {
    eventInstruction->notify(disassembleInstruction(), disassembleContext());
  }
  instruction();
}

auto ArmDSP::step(uint clocks) -> void {
  if(bridge.timer && --bridge.timer == 0);
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto ArmDSP::power() -> void {
  random.array((uint8*)programRAM, sizeof(programRAM));
  bridge.reset = false;
  reset();
}

auto ArmDSP::reset() -> void {
  ARM7TDMI::power();
  Thread::create(Frequency, [&] {
    boot();
    while(true) scheduler.synchronize(), main();
  });
  cpu.coprocessors.append(this);

  bridge.ready = false;
  bridge.signal = false;
  bridge.timer = 0;
  bridge.timerlatch = 0;
  bridge.cputoarm.ready = false;
  bridge.armtocpu.ready = false;
}
