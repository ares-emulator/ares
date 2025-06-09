#include <gb/gb.hpp>

namespace ares::GameBoy {

#define SP r.sp.word
#define PC r.pc.word

#include "io.cpp"
#include "memory.cpp"
#include "timing.cpp"
#include "debugger.cpp"
#include "serialization.cpp"
CPU cpu;

auto CPU::load(Node::Object parent) -> void {
  wram.allocate(!Model::GameBoyColor() ? 8_KiB : 32_KiB);
  hram.allocate(128);

  node = parent->append<Node::Object>("CPU");

  if(Model::GameBoy()) {
    version = node->append<Node::Setting::String>("Version", "DMG-CPU B");
    version->setAllowedValues({
      "DMG-CPU",
      "DMG-CPU A",
      "DMG-CPU B",
      "DMG-CPU C",
      "CPU MGB",
    });
  }

  if(Model::SuperGameBoy()) {
    version = node->append<Node::Setting::String>("Version", "SGB-CPU 01");
    version->setAllowedValues({
      "SGB-CPU 01",
      "CPU SGB2"
    });
  }

  if(Model::GameBoyColor()) {
    version = node->append<Node::Setting::String>("Version", "CPU CGB");
    version->setAllowedValues({
      "CPU CGB",
      "CPU CGB A",
      "CPU CGB B",
      "CPU CGB C",
      "CPU CGB D",
      "CPU CGB E",
    });
  }

  debugger.load(node);
}

auto CPU::unload() -> void {
  wram.reset();
  hram.reset();
  node = {};
  version = {};
  debugger = {};
}

auto CPU::main() -> void {
  if(ppu.status.ly < 144 && status.hdmaPending) {
    performHdma();
    status.hdmaPending = 0;
  }

  //are interrupts enabled?
  if(r.ime) {
    //are any interrupts pending?
    if(status.interruptLatch) {
      debugger.interrupt("IRQ");

      idle();
      idle();
      idle();
      r.ime = 0;
      write(--SP, PC >> 8);  //upper byte may write to IE before it is polled again
      n8 mask = status.interruptFlag & status.interruptEnable;
      write(--SP, PC >> 0);  //lower byte write to IE has no effect
      if(mask) {
        u32 interruptID = bit::first(mask);  //find highest priority interrupt
        lower(interruptID);
        PC = 0x0040 + interruptID * 8;
      } else {
        //if push(PCH) writes to IE and disables all requested interrupts, PC is forced to zero
        PC = 0x0000;
      }
    }
  }

  debugger.instruction();
  instruction();

  if(Model::SuperGameBoy()) {
    scheduler.exit(Event::Step);
  }
}

auto CPU::raised(u32 interruptID) const -> bool {
  return status.interruptFlag.bit(interruptID);
}

auto CPU::raise(u32 interruptID) -> void {
  status.interruptFlag.bit(interruptID) = 1;
  if(status.interruptEnable.bit(interruptID)) {
    r.halt = false;
    if(interruptID == Interrupt::Joypad) r.stop = false;
  }
}

auto CPU::lower(u32 interruptID) -> void {
  status.interruptFlag.bit(interruptID) = 0;
}

auto CPU::stoppable() -> bool {
  status.div = 0;

  if(status.speedSwitch) {
    status.speedSwitch = 0;
    status.speedDouble ^= 1;
    if(status.speedDouble == 0) setFrequency(4 * 1024 * 1024);
    if(status.speedDouble == 1) setFrequency(8 * 1024 * 1024);
    return false;
  }

  return true;
}

auto CPU::power() -> void {
  Thread::create(4 * 1024 * 1024, {&CPU::main, this});
  SM83::power();

  for(auto& n : wram) n = 0x00;
  for(auto& n : hram) n = 0x00;

  status = {};

  // TODO: Validate this for models other than dmg ABC/mgb
  status.div = 8;

  if(Model::GameBoyColor()) status.cgbMode = 1;
}

}
