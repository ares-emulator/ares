#include <n64/n64.hpp>

namespace ares::Nintendo64 {

auto load(Node::System& node, string name) -> bool {
  return system.load(node, name);
}

System system;
#include "serialization.cpp"

auto System::game() -> string {
  if(cartridge.node) {
    return cartridge.name();
  }

  return "(no cartridge connected)";
}

auto System::run() -> void {
  while(!vi.refreshed) cpu.main();
  vi.refreshed = false;
  si.main();
}

auto System::load(Node::System& root, string name) -> bool {
  if(node) unload();

  information = {};

  node = Node::System::create(name);
  node->setGame({&System::game, this});
  node->setRun({&System::run, this});
  node->setPower({&System::power, this});
  node->setSave({&System::save, this});
  node->setUnload({&System::unload, this});
  node->setSerialize({&System::serialize, this});
  node->setUnserialize({&System::unserialize, this});
  root = node;

  regionNode = node->append<Node::Setting::String>("Region", "NTSC → PAL");
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
  return true;
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
  for(auto& setting : node->find<Node::Setting::Setting>()) setting->setLatch();

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

  //zero-initialization
  if(!reset) {
    serializer s;
    s.setReading();
    serialize(s, true);
  }

  cartridge.power(reset);
  dd.power(reset);
  mi.power(reset);
  vi.power(reset);
  ai.power(reset);
  pi.power(reset);
  ri.power(reset);
  si.power(reset);
  rdram.power(reset);
  cpu.power(reset);
  rdp.power(reset);
  rsp.power(reset);
}

}
