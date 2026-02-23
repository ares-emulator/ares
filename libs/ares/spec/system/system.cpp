#include <spec/spec.hpp>
#include <algorithm>

namespace ares::ZXSpectrum {

Scheduler scheduler;
ROM rom;
System system;
#include "serialization.cpp"

auto enumerate() -> std::vector<string> {
  return {
    "[Sinclair] ZX Spectrum",
    "[Sinclair] ZX Spectrum 128",
  };
}

auto load(Node::System& node, string name) -> bool {
  auto list = enumerate();
  if(std::find(list.begin(), list.end(), name) == list.end()) return false;
  return system.load(node, name);
}

auto System::run() -> void {
  scheduler.enter();
}

auto System::game() -> string {
  if(tapeDeck.tray.tape) {
    return tapeDeck.tray.tape.title();
  }

  return "(no tape inserted)";
}

auto System::load(Node::System& root, string name) -> bool {
  if(node) unload();

  information = {};

  if(name.find("ZX Spectrum 128")) {
    information.name = "ZX Spectrum 128";
    information.model = Model::Spectrum128;
    information.frequency = 3'546'900;
  } else if(name.find("ZX Spectrum")) {
    information.name = "ZX Spectrum";
    information.model = Model::Spectrum48k;
    information.frequency = 3'500'000;
  }

  node = std::make_shared<Core::System>(information.name);
  node->setAttribute("configuration", name);
  node->setGame(std::bind_front(&System::game, this));
  node->setRun(std::bind_front(&System::run, this));
  node->setPower(std::bind_front(&System::power, this));
  node->setSave(std::bind_front(&System::save, this));
  node->setUnload(std::bind_front(&System::unload, this));
  node->setSerialize([this](bool save) -> serializer { return serialize(save); });
  node->setUnserialize(std::bind_front(&System::unserialize, this));
  root = node;
  if(!node->setPak(pak = platform->pak(node))) return false;

  scheduler.reset();
  cpu.load(node);
  tapeDeck.load(node);
  keyboard.load(node);
  expansionPort.load(node);
  ula.load(node);

  if (model() == Model::Spectrum128) {
    psg.load(node);
  }

  return true;
}

auto System::save() -> void {
  if(!node) return;
}

auto System::unload() -> void {
  if(!node) return;
  save();
  cpu.unload();
  ula.unload();
  keyboard.unload();
  tapeDeck.unload();
  expansionPort.unload();
  node = {};
  rom.bios.reset();

  if (model() == Model::Spectrum128) {
    psg.unload();
    rom.sub.reset();
  }
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting::Setting>()) setting->setLatch();

  rom.bios.allocate(16_KiB);
  if(auto fp = pak->read("bios.rom")) {
    rom.bios.load(fp);
  }

  if (model() == Model::Spectrum128) {
    rom.sub.allocate(16_KiB);
    if(auto fp = pak->read("sub.rom")) {
      rom.sub.load(fp);
    }
  }

  romBank = 0;
  ramBank = 0;
  screenBank = 0;
  pagingDisabled = 0;

  cpu.power();
  keyboard.power();
  ula.power();
  if (model() == Model::Spectrum128) {
    psg.power();
  }
  tapeDeck.power();
  scheduler.power(cpu);

  information.serializeSize[0] = serializeInit(0);
  information.serializeSize[1] = serializeInit(1);
}

}
