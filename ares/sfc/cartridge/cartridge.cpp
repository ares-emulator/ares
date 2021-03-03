#include <sfc/sfc.hpp>

namespace ares::SuperFamicom {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "slot.cpp"
#include "load.cpp"
#include "save.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  node = parent->append<Node::Peripheral>(string{system.name(), " Cartridge"});
  debugger.load(node);
  return node;
}

auto Cartridge::connect() -> void {
  if(!node->setPak(pak = platform->pak(node))) return;

  information = {};
  has = {};
  information.title  = pak->attribute("title");
  information.region = pak->attribute("region");
  information.board  = pak->attribute("board");

  loadCartridge();
  if(has.SA1) sa1.load(node);
  if(has.SuperFX) superfx.load(node);
  if(has.ARMDSP) armdsp.load(node);
  if(has.HitachiDSP) hitachidsp.load(node);
  if(has.NECDSP) necdsp.load(node);
  if(has.EpsonRTC) epsonrtc.load(node);
  if(has.SharpRTC) sharprtc.load(node);
  if(has.MSU1) msu1.load(node);
  if(has.GameBoySlot) icd.load(node);
  if(has.BSMemorySlot) bsmemorySlot.load(node);
  if(has.SufamiTurboSlotA) sufamiturboSlotA.load(node);
  if(has.SufamiTurboSlotB) sufamiturboSlotB.load(node);
  power(false);
}

auto Cartridge::disconnect() -> void {
  if(!node) return;

  if(has.ICD) icd.unload();
  if(has.MCC) mcc.unload();
  if(has.Competition) competition.unload();
  if(has.SA1) sa1.unload();
  if(has.SuperFX) superfx.unload();
  if(has.ARMDSP) armdsp.unload();
  if(has.HitachiDSP) hitachidsp.unload();
  if(has.NECDSP) necdsp.unload();
  if(has.EpsonRTC) epsonrtc.unload();
  if(has.SharpRTC) sharprtc.unload();
  if(has.SPC7110) spc7110.unload();
  if(has.SDD1) sdd1.unload();
  if(has.OBC1) obc1.unload();
  if(has.MSU1) msu1.unload(node);
  if(has.BSMemorySlot) bsmemorySlot.unload();
  if(has.SufamiTurboSlotA) sufamiturboSlotA.unload();
  if(has.SufamiTurboSlotB) sufamiturboSlotB.unload();

  rom.reset();
  ram.reset();
  bus.reset();
  pak.reset();
  node.reset();
}

auto Cartridge::power(bool reset) -> void {
  if(has.ICD) icd.power();
  if(has.MCC) mcc.power();
  if(has.DIP) dip.power();
  if(has.Competition) competition.power();
  if(has.SA1) sa1.power();
  if(has.SuperFX) superfx.power();
  if(has.ARMDSP) armdsp.power();
  if(has.HitachiDSP) hitachidsp.power();
  if(has.NECDSP) necdsp.power();
  if(has.EpsonRTC) epsonrtc.power();
  if(has.SharpRTC) sharprtc.power();
  if(has.SPC7110) spc7110.power();
  if(has.SDD1) sdd1.power();
  if(has.OBC1) obc1.power();
  if(has.MSU1) msu1.power();
  if(has.BSMemorySlot) bsmemory.power();
  if(has.SufamiTurboSlotA) sufamiturboA.power();
  if(has.SufamiTurboSlotB) sufamiturboB.power();
}

auto Cartridge::save() -> void {
  if(!node) return;

  saveCartridge();
  if(has.GameBoySlot) icd.save();
  if(has.BSMemorySlot) bsmemory.save();
  if(has.SufamiTurboSlotA) sufamiturboA.save();
  if(has.SufamiTurboSlotB) sufamiturboB.save();
}

}
