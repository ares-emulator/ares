#include <n64/n64.hpp>

namespace ares::Nintendo64 {

System system;
#include "serialization.cpp"

auto System::run() -> void {
  while(!vi.refreshed) cpu.main();
  vi.refreshed = false;
  vi.refresh();
  si.main();
}

auto System::load(Node::Object& root) -> void {
  if(node) unload();

  information = {};

  node = Node::System::create(interface->name());
  root = node;

  regionNode = node->append<Node::String>("Region", "NTSC → PAL");
  regionNode->setAllowedValues({
    "NTSC → PAL",
    "PAL → NTSC",
    "NTSC",
    "PAL"
  });

  mi.load(node);
  vi.load(node);
  ai.load(node);
  pi.load(node);
  ri.load(node);
  si.load(node);
  rdram.load(node);
  cpu.load(node);
  rdp.load(node);
  rsp.load(node);
  cartridgeSlot.load(node);
  controllerPort1.load(node);
  controllerPort2.load(node);
  controllerPort3.load(node);
  controllerPort4.load(node);
  dd.load(node);
}

auto System::unload() -> void {
  if(!node) return;
  save();
  mi.unload();
  vi.unload();
  ai.unload();
  pi.unload();
  ri.unload();
  si.unload();
  rdram.unload();
  cpu.unload();
  rdp.unload();
  rsp.unload();
  cartridgeSlot.unload();
  controllerPort1.unload();
  controllerPort2.unload();
  controllerPort3.unload();
  controllerPort4.unload();
  dd.unload();
  node.reset();
}

auto System::save() -> void {
  if(!node) return;
  cartridge.save();
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting>()) setting->setLatch();

  auto setRegion = [&](string region) {
    if(region == "NTSC") {
      information.region = Region::NTSC;
    }
    if(region == "PAL") {
      information.region = Region::PAL;
    }
  };
  auto regionsHave = regionNode->latch().split("→").strip();
  setRegion(regionsHave.first());
  for(auto& have : reverse(regionsHave)) {
    if(have == cartridge.region()) setRegion(have);
  }

  cartridge.power();
  dd.power(reset);
  mi.power();
  vi.power();
  ai.power();
  pi.power();
  ri.power();
  si.power();
  rdram.power();
  cpu.power(reset);
  rdp.power();
  rsp.power();

  information.serializeSize[0] = serializeInit(0);
  information.serializeSize[1] = serializeInit(1);
}

}
