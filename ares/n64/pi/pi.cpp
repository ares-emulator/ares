#include <n64/n64.hpp>

namespace ares::Nintendo64 {

PI pi;
#include "dma.cpp"
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto PI::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("PI");
  rom.allocate(0x7c0);
  ram.allocate(0x040);

  debugger.load(node);
}

auto PI::unload() -> void {
  debugger = {};
  rom.reset();
  ram.reset();
  node.reset();
}

auto PI::power(bool reset) -> void {
  string pifrom = cartridge.region() == "NTSC" ? "pif.ntsc.rom" : "pif.pal.rom";
  if(auto fp = system.pak->read(pifrom)) {
    rom.load(fp);
  }

  ram.fill();
  io = {};
  bsd1 = {};
  bsd2 = {};

  //write CIC seeds into PIF RAM so that cartridge checksum function passes
  //d0-d7  = CIC IPL2 seed
  //d8-d15 = CIC IPL3 seed
  //d17    = osResetType (0 = power; 1 = reset (NMI))
  //d18    = osVersion
  //d19    = osRomType (0 = Gamepak; 1 = 64DD)
  string cic = cartridge.cic();

  //FIXME: https://github.com/higan-emu/ares/issues/61
  //This is currently a workaround to get both Battletanx (U) games to boot. PIF RAM offset 0x24 is where IPL2 reads a bunch of status information from the PIF. 
  //"adding" 0x20000 is the same as setting bit 17. This bit is a flag that states whether the console is starting from cold boot, or NMI (a "soft" reset). 0=cold reset. 1=NMI.
  //Additionally, similar to other bits of data from this word, this particular bit is shifted into the bit 0 position, and stored in CPU register s5 for possible use in IPL3 
  //or further game code. Unfortunately the boot process is currently not well understood. This is likely masking an initialization error when powering on from a cold boot 
  //within the emulator. If this issue is identified and addressed, then we should check and make use of the reset flag properly here (remove the comment from the next line). 
  if(/*reset == */0) {
    if(cic == "CIC-NUS-6101" || cic == "CIC-NUS-7102") ram.write<Word>(0x24, 0x00043f3f);
    if(cic == "CIC-NUS-6102" || cic == "CIC-NUS-7101") ram.write<Word>(0x24, 0x00003f3f);
    if(cic == "CIC-NUS-6103" || cic == "CIC-NUS-7103") ram.write<Word>(0x24, 0x0000783f);
    if(cic == "CIC-NUS-6105" || cic == "CIC-NUS-7105") ram.write<Word>(0x24, 0x0000913f);
    if(cic == "CIC-NUS-6106" || cic == "CIC-NUS-7106") ram.write<Word>(0x24, 0x0000853f);
  }
  else
  {
    if(cic == "CIC-NUS-6101" || cic == "CIC-NUS-7102") ram.write<Word>(0x24, 0x00063f3f);
    if(cic == "CIC-NUS-6102" || cic == "CIC-NUS-7101") ram.write<Word>(0x24, 0x00023f3f);
    if(cic == "CIC-NUS-6103" || cic == "CIC-NUS-7103") ram.write<Word>(0x24, 0x0002783f);
    if(cic == "CIC-NUS-6105" || cic == "CIC-NUS-7105") ram.write<Word>(0x24, 0x0002913f);
    if(cic == "CIC-NUS-6106" || cic == "CIC-NUS-7106") ram.write<Word>(0x24, 0x0002853f);
  }
}

}
