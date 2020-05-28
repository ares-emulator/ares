#include <n64/n64.hpp>

namespace ares::Nintendo64 {

PI pi;
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto PI::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("PI");
  rom.allocate(0x7c0);
  ram.allocate(0x040);

  debugger.load(node);
}

auto PI::unload() -> void {
  rom.reset();
  ram.reset();
  node = {};
  debugger = {};
}

auto PI::power() -> void {
  rom.fill();
  ram.fill();
  io = {};
  bsd1 = {};
  bsd2 = {};

  string iplrom = cartridge.region() == "NTSC" ? "pif.ntsc.rom" : "pif.pal.rom";
  iplrom = "pif.rom";
  if(auto fp = platform->open(node, iplrom, File::Read, File::Required)) {
    rom.load(fp);
  }

  //write CIC seeds into PIF RAM so that cartridge checksum function passes
  string cic = cartridge.cic();
  if(1) {  //power
    if(cic == "CIC-NUS-6101" || cic == "CIC-NUS-7102") ram.writeWord(0x24, 0x00043f3f);
    if(cic == "CIC-NUS-6102" || cic == "CIC-NUS-7101") ram.writeWord(0x24, 0x00003f3f);
    if(cic == "CIC-NUS-6103" || cic == "CIC-NUS-7103") ram.writeWord(0x24, 0x0000783f);
    if(cic == "CIC-NUS-6105" || cic == "CIC-NUS-7105") ram.writeWord(0x24, 0x0000913f);
    if(cic == "CIC-NUS-6106" || cic == "CIC-NUS-7106") ram.writeWord(0x24, 0x0000853f);
  }

  if(0) {  //reset
    if(cic == "CIC-NUS-6101" || cic == "CIC-NUS-7102") ram.writeWord(0x24, 0x00063f3f);
    if(cic == "CIC-NUS-6102" || cic == "CIC-NUS-7101") ram.writeWord(0x24, 0x00023f3f);
    if(cic == "CIC-NUS-6103" || cic == "CIC-NUS-7103") ram.writeWord(0x24, 0x0002783f);
    if(cic == "CIC-NUS-6105" || cic == "CIC-NUS-7105") ram.writeWord(0x24, 0x0002913f);
    if(cic == "CIC-NUS-6106" || cic == "CIC-NUS-7106") ram.writeWord(0x24, 0x0002853f);
  }
}

}
