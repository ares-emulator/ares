#include <sfc/sfc.hpp>

namespace ares::SuperFamicom {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "slot.cpp"
#include "load.cpp"
#include "save.cpp"
#include "serialization.cpp"

auto Cartridge::connect(Node::Port parent, Node::Peripheral with) -> void {
  node = Node::append<Node::Peripheral>(parent, with, interface->name());
  node->setManifest([&] { return information.manifest; });

  information = {};
  has = {};

  if(auto fp = platform->open(node, "manifest.bml", File::Read, File::Required)) {
    information.manifest = fp->reads();
    information.document = BML::unserialize(information.manifest);
    information.name     = information.document["game/label"].text();
    information.region   = information.document["game/region"].text();
    information.board    = information.document["game/board"].text();
  }

  loadCartridge(information.document);
  if(has.SA1) sa1.load(node, with);
  if(has.SuperFX) superfx.load(node, with);
  if(has.ARMDSP) armdsp.load(node, with);
  if(has.HitachiDSP) hitachidsp.load(node, with);
  if(has.NECDSP) necdsp.load(node, with);
  if(has.MSU1) msu1.load(node, with);
  if(has.GameBoySlot) icd.load(node, with);
  if(has.BSMemorySlot) bsmemory.load(node, with);
  if(has.SufamiTurboSlotA) sufamiturboA.load(node, with);
  if(has.SufamiTurboSlotB) sufamiturboB.load(node, with);

  if(has.EpsonRTC) epsonrtc.load(node, with);
  if(has.SharpRTC) sharprtc.load(node, with);

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
  if(has.MSU1) msu1.unload();
  if(has.BSMemorySlot) bsmemory.unload();
  if(has.SufamiTurboSlotA) sufamiturboA.unload();
  if(has.SufamiTurboSlotB) sufamiturboB.unload();

  rom.reset();
  ram.reset();
  bus.reset();
  node = {};
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

  saveCartridge(information.document);
  if(has.GameBoySlot);  //todo
  if(has.BSMemorySlot) bsmemory.save();
  if(has.SufamiTurboSlotA) sufamiturboA.save();
  if(has.SufamiTurboSlotB) sufamiturboB.save();
}

auto Cartridge::lookupMemory(Markup::Node memory) -> Markup::Node {
  for(auto node : information.document.find("game/board/memory")) {
    if(memory["type"        ] && memory["type"        ].text()    != node["type"        ].text()   ) continue;
    if(memory["size"        ] && memory["size"        ].natural() != node["size"        ].natural()) continue;
    if(memory["content"     ] && memory["content"     ].text()    != node["content"     ].text()   ) continue;
    if(memory["manufacturer"] && memory["manufacturer"].text()    != node["manufacturer"].text()   ) continue;
    if(memory["architecture"] && memory["architecture"].text()    != node["architecture"].text()   ) continue;
    if(memory["identifier"  ] && memory["identifier"  ].text()    != node["identifier"  ].text()   ) continue;
    return node;
  }
  return {};
}

//note: there are currently no oscillator identifiers:
//it's presumed that there is never more than one oscillator on the same board,
//and so the first oscillator is returned instead for now.
auto Cartridge::lookupOscillator() -> Markup::Node {
  for(auto node : information.document.find("game/board/oscillator")) {
    return node;
  }
  return {};
}

}
