#include <sfc/interface/interface.hpp>

struct SuperFamicom : Emulator {
  SuperFamicom();
  auto load() -> bool override;
  auto open(ares::Node::Object, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> override;
  auto input(ares::Node::Input) -> void override;
};

SuperFamicom::SuperFamicom() {
  interface = new ares::SuperFamicom::SuperFamicomInterface;
  medium = mia::medium("Super Famicom");
  manufacturer = "Nintendo";
  name = "Super Famicom";
}

auto SuperFamicom::load() -> bool {
  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Gamepad");
    port->connect();
  }

  return true;
}

auto SuperFamicom::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(name == "manifest.bml") return Emulator::manifest();

  if(name == "boards.bml") {
    return vfs::memory::open(Resource::SuperFamicom::Boards, sizeof Resource::SuperFamicom::Boards);
  }

  if(name == "ipl.rom") {
    return vfs::memory::open(Resource::SuperFamicom::IPLROM, sizeof Resource::SuperFamicom::IPLROM);
  }

  auto document = BML::unserialize(game.manifest);
  auto programROMSize = document["game/board/memory(content=Program,type=ROM)/size"].natural();
  auto dataROMSize = document["game/board/memory(content=Data,type=ROM)/size"].natural();
  auto expansionROMSize = document["game/board/memory(content=Expansion.type=ROM)/size"].natural();

  if(name == "program.rom") {
    return vfs::memory::open(game.image.data(), programROMSize);
  }

  if(name == "data.rom") {
    return vfs::memory::open(game.image.data() + programROMSize, dataROMSize);
  }

  if(name == "expansion.rom") {
    return vfs::memory::open(game.image.data() + programROMSize + dataROMSize, expansionROMSize);
  }

  if(name == "save.ram") {
    if((bool)document["game/board/memory(content=Save,type=RAM)/volatile"]) return {};
    auto location = locate(game.location, ".sav", settings.paths.saves);
    if(auto result = vfs::disk::open(location, mode)) return result;
  }

  if(name == "download.ram") {
    auto location = locate(game.location, ".psr", settings.paths.saves);
    if(auto result = vfs::disk::open(location, mode)) return result;
  }

  if(name == "time.rtc") {
    auto location = locate(game.location, ".rtc", settings.paths.saves);
    if(auto result = vfs::disk::open(location, mode)) return result;
  }

  if(name.beginsWith("upd7725.")) {
    auto identifier = document["game/board/memory(architecture=uPD7725)/identifier"].string().downcase();
    auto firmwareProgramROMSize = document["game/board/memory(content=Program,type=ROM,architecture=uPD7725)/size"].natural();
    auto firmwareDataROMSize = document["game/board/memory(content=Data,type=ROM,architecture=uPD7725)/size"].natural();

    if(name == "upd7725.program.rom") {
      if(game.image.size() >= programROMSize + firmwareProgramROMSize) {
        return vfs::memory::open(game.image.data() + programROMSize, firmwareProgramROMSize);
      }
      auto location = locate({Location::dir(game.location), identifier, ".program.rom"}, ".rom", settings.paths.firmware);
      if(auto result = vfs::disk::open(location, mode)) return result;
      error({"Missing required file: ", identifier, ".program.rom"});
    }

    if(name == "upd7725.data.rom") {
      if(game.image.size() >= programROMSize + firmwareProgramROMSize + firmwareDataROMSize) {
        return vfs::memory::open(game.image.data() + programROMSize + firmwareProgramROMSize, firmwareDataROMSize);
      }
      auto location = locate({Location::dir(game.location), identifier, ".data.rom"}, ".rom", settings.paths.firmware);
      if(auto result = vfs::disk::open(location, mode)) return result;
      error({"Missing required file: ", identifier, ".data.rom"});
    }

    if(name == "upd7725.data.ram") {
      if((bool)document["game/board/memory(content=Data,type=RAM,architecture=uPD7725)/volatile"]) return {};
      auto location = locate(game.location, ".upd7725.sav", settings.paths.saves);
      if(auto result = vfs::disk::open(location, mode)) return result;
    }
  }

  if(name.beginsWith("upd96050.")) {
    auto identifier = document["game/board/memory(architecture=uPD96050)/identifier"].string().downcase();
    auto firmwareProgramROMSize = document["game/board/memory(content=Program,type=ROM,architecture=uPD96050)/size"].natural();
    auto firmwareDataROMSize = document["game/board/memory(content=Program,type=ROM,architecture=uPD96050)/size"].natural();

    if(name == "upd96050.program.rom") {
      if(game.image.size() >= programROMSize + firmwareProgramROMSize) {
        return vfs::memory::open(game.image.data() + programROMSize, firmwareProgramROMSize);
      }
      auto location = locate({Location::dir(game.location), identifier, ".program.rom"}, ".rom", settings.paths.firmware);
      if(auto result = vfs::disk::open(location, mode)) return result;
      error({"Missing required file: ", identifier, ".program.rom"});
    }

    if(name == "upd96050.data.rom") {
      if(game.image.size() >= programROMSize + firmwareProgramROMSize + firmwareDataROMSize) {
        return vfs::memory::open(game.image.data() + programROMSize + firmwareProgramROMSize, firmwareDataROMSize);
      }
      auto location = locate({Location::dir(game.location), identifier, ".data.rom"}, ".rom", settings.paths.firmware);
      if(auto result = vfs::disk::open(location, mode)) return result;
      error({"Missing required file: ", identifier, ".data.rom"});
    }

    if(name == "upd96050.data.ram") {
      if((bool)document["game/board/memory(content=Data,type=RAM,architecture=uPD96050)/volatile"]) return {};
      auto location = locate(game.location, ".upd96050.sav", settings.paths.saves);
      if(auto result = vfs::disk::open(location, mode)) return result;
    }
  }

  if(name.beginsWith("hg51bs169.")) {
    auto identifier = document["game/board/memory(architecture=HG51BS169)/identifier"].string().downcase();
    auto firmwareDataROMSize = document["game/board/memory(content=Data,type=ROM,architecture=HG51BS169)/size"].natural();

    if(name == "hg51bs169.data.rom") {
      if(game.image.size() >= programROMSize + firmwareDataROMSize) {
        return vfs::memory::open(game.image.data() + programROMSize, firmwareDataROMSize);
      }
      auto location = locate({Location::dir(game.location), identifier, ".data.rom"}, ".rom", settings.paths.firmware);
      if(auto result = vfs::disk::open(location, mode)) return result;
      error({"Missing required file: ", identifier, ".data.rom"});
    }

    if(name == "hg51bs169.data.ram") {
      if((bool)document["game/board/memory(content=Data,type=RAM,architecture=HG51BS169)/volatile"]) return {};
      auto location = locate(game.location, ".hg51bs169.sav", settings.paths.saves);
      if(auto result = vfs::disk::open(location, mode)) return result;
    }
  }

  if(name.beginsWith("arm6.")) {
    auto identifier = document["game/board/memory(architecture=ARM6)/identifier"].string().downcase();
    auto firmwareProgramROMSize = document["game/board/memory(content=Program,type=ROM,architecture=ARM6)/size"].natural();
    auto firmwareDataROMSize = document["game/board/memory(content=Data,type=ROM,architecture=ARM6)/size"].natural();

    if(name == "arm6.program.rom") {
      if(game.image.size() >= programROMSize + firmwareProgramROMSize) {
        return vfs::memory::open(game.image.data() + programROMSize, firmwareProgramROMSize);
      }
      auto location = locate({Location::dir(game.location), identifier, ".program.rom"}, ".rom", settings.paths.firmware);
      if(auto result = vfs::disk::open(location, mode)) return result;
      error({"Missing required file: ", identifier, ".program.rom"});
    }

    if(name == "arm6.data.rom") {
      if(game.image.size() >= programROMSize + firmwareProgramROMSize + firmwareDataROMSize) {
        return vfs::memory::open(game.image.data() + programROMSize + firmwareProgramROMSize, firmwareDataROMSize);
      }
      auto location = locate({Location::dir(game.location), identifier, ".data.rom"}, ".rom", settings.paths.firmware);
      if(auto result = vfs::disk::open(location, mode)) return result;
      error({"Missing required file: ", identifier, ".data.rom"});
    }

    if(name == "arm6.data.ram") {
      if((bool)document["game/board/memory(content=Data,type=RAM,architecture=ARM6)/volatile"]) return {};
      auto location = locate(game.location, ".arm6.sav", settings.paths.saves);
      if(auto result = vfs::disk::open(location, mode)) return result;
    }
  }

  if(name == "msu1/data.rom") {
    auto location = locate(game.location, ".msu");
    if(auto result = vfs::disk::open(location, mode)) return result;
  }

  if(name.beginsWith("msu1/track-")) {
    auto location = locate({game.location, string{name}.trimLeft("msu1/", 1L)}, ".pcm");
    if(auto result = vfs::disk::open(location, mode)) return result;
  }

  return {};
}

auto SuperFamicom::input(ares::Node::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "Up"    ) mapping = virtualPad.up;
  if(name == "Down"  ) mapping = virtualPad.down;
  if(name == "Left"  ) mapping = virtualPad.left;
  if(name == "Right" ) mapping = virtualPad.right;
  if(name == "B"     ) mapping = virtualPad.a;
  if(name == "A"     ) mapping = virtualPad.b;
  if(name == "Y"     ) mapping = virtualPad.x;
  if(name == "X"     ) mapping = virtualPad.y;
  if(name == "L"     ) mapping = virtualPad.l;
  if(name == "R"     ) mapping = virtualPad.r;
  if(name == "Select") mapping = virtualPad.select;
  if(name == "Start" ) mapping = virtualPad.start;

  if(mapping) {
    auto value = mapping->value();
    if(auto button = node->cast<ares::Node::Button>()) {
      button->setValue(value);
    }
  }
}
