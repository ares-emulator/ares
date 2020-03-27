#include <sfc/sfc.hpp>

namespace ares::SuperFamicom {

SMP smp;
#include "memory.cpp"
#include "io.cpp"
#include "timing.cpp"
#include "serialization.cpp"

auto SMP::load(Node::Object parent, Node::Object from) -> void {
  node = Node::append<Node::Component>(parent, from, "SMP");
  from = Node::scan(parent = node, from);

  debugInstruction = Node::append<Node::Instruction>(parent, from, "Instruction", "SMP");
  debugInstruction->setAddressBits(16);
}

auto SMP::unload() -> void {
  debugInstruction = {};
  node = {};
}

auto SMP::main() -> void {
  if(r.wait) return instructionWait();
  if(r.stop) return instructionStop();

  if(debugInstruction->enabled() && debugInstruction->address(r.pc.w)) {
    debugInstruction->notify(disassembleInstruction(), disassembleContext());
  }
  instruction();
}

auto SMP::power(bool reset) -> void {
  if(auto fp = platform->open(system.node, "ipl.rom", File::Read, File::Required)) {
    fp->read(iplrom, 64);
  }

  SPC700::power();
  create(system.apuFrequency() / 12.0, {&SMP::main, this});

  r.pc.byte.l = iplrom[62];
  r.pc.byte.h = iplrom[63];

  io = {};
  timer0 = {};
  timer1 = {};
  timer2 = {};
}

}
