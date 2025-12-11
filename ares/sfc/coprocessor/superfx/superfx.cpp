SuperFX superfx;
#include "bus.cpp"
#include "core.cpp"
#include "memory.cpp"
#include "io.cpp"
#include "timing.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto SuperFX::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("SuperFX");

  debugger.load(node);
}

auto SuperFX::unload() -> void {
  debugger = {};
  node = {};

  rom.reset();
  ram.reset();
  bram.reset();

  std::erase(cpu.coprocessors, this);
  Thread::destroy();
}

auto SuperFX::main() -> void {
  if(regs.sfr.g == 0) return step(6);

  auto opcode = peekpipe();
  debugger.instruction();
  instruction(opcode);

  if(regs.r[14].modified) {
    regs.r[14].modified = false;
    updateROMBuffer();
  }

  if(regs.r[15].modified) {
    regs.r[15].modified = false;
  } else {
    regs.r[15]++;
  }
}

// TODO: find a better place for this routine
auto romSizeRound(u32 romSize) -> u32 {
  u32 count = 0;
  if (romSize && !(romSize & (romSize - 1))) return romSize;
  while(romSize != 0) {
    romSize >>= 1;
    count += 1;
  }
  return 1 << count;
}

auto SuperFX::power() -> void {
  GSU::power();

  Thread::create(Frequency, std::bind_front(&SuperFX::main, this));
  cpu.coprocessors.push_back(this);

  // SuperFX voxel demo has a non power-of-two rom
  // resulting in an incorrect/broken rom mask
  // fix this by ensuring romSize is a power of two
  romMask = romSizeRound(rom.size()) - 1;
  ramMask = ram.size() - 1;
  bramMask = bram.size() - 1;

  for(u32 n : range(512)) cache.buffer[n] = 0x00;
  for(u32 n : range(32)) cache.valid[n] = false;
  for(u32 n : range(2)) {
    pixelcache[n].offset = ~0;
    pixelcache[n].bitpend = 0x00;
  }

  regs.romcl = 0;
  regs.romdr = 0;

  regs.ramcl = 0;
  regs.ramar = 0;
  regs.ramdr = 0;
}
