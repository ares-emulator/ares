namespace ares::GameBoy {
  auto load(Node::System& node, string name) -> bool;
}

struct GameBoy : Emulator {
  GameBoy();
  auto load() -> bool override;
  auto open(ares::Node::Object, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> override;
  auto input(ares::Node::Input::Input) -> void override;
};

struct GameBoyColor : Emulator {
  GameBoyColor();
  auto load() -> bool override;
  auto open(ares::Node::Object, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> override;
  auto input(ares::Node::Input::Input) -> void override;
};

GameBoy::GameBoy() {
  medium = mia::medium("Game Boy");
  manufacturer = "Nintendo";
  name = "Game Boy";
}

auto GameBoy::load() -> bool {
  if(!ares::GameBoy::load(root, "[Nintendo] Game Boy")) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto fastBoot = root->find<ares::Node::Setting::Boolean>("Fast Boot")) {
    fastBoot->setValue(settings.boot.fast);
  }

  return true;
}

auto GameBoy::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(name == "manifest.bml") return Emulator::manifest();

  if(name == "boot.dmg-1.rom") {
    return vfs::memory::open(Resource::GameBoy::BootDMG1, sizeof Resource::GameBoy::BootDMG1);
  }

  auto document = BML::unserialize(game.manifest);
  auto programROMSize = document["game/board/memory(content=Program,type=ROM)/size"].natural();
  auto saveRAMVolatile = (bool)document["game/board/memory(Content=Save,type=RAM)/volatile"];

  if(name == "program.rom") {
    return vfs::memory::open(game.image.data(), programROMSize);
  }

  if(name == "save.ram" && !saveRAMVolatile) {
    auto location = locate(game.location, ".sav", settings.paths.saves);
    if(auto result = vfs::disk::open(location, mode)) return result;
  }

  if(name == "save.eeprom") {
    auto location = locate(game.location, ".sav", settings.paths.saves);
    if(auto result = vfs::disk::open(location, mode)) return result;
  }

  if(name == "download.flash") {
    auto location = locate(game.location, ".flash", settings.paths.saves);
    if(auto result = vfs::disk::open(location, mode)) return result;
  }

  if(name == "time.rtc") {
    auto location = locate(game.location, ".rtc", settings.paths.saves);
    if(auto result = vfs::disk::open(location, mode)) return result;
  }

  return {};
}

auto GameBoy::input(ares::Node::Input::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "Up"    ) mapping = virtualPads[0].up;
  if(name == "Down"  ) mapping = virtualPads[0].down;
  if(name == "Left"  ) mapping = virtualPads[0].left;
  if(name == "Right" ) mapping = virtualPads[0].right;
  if(name == "B"     ) mapping = virtualPads[0].a;
  if(name == "A"     ) mapping = virtualPads[0].b;
  if(name == "Select") mapping = virtualPads[0].select;
  if(name == "Start" ) mapping = virtualPads[0].start;
  //MBC5
  if(name == "Rumble") mapping = virtualPads[0].rumble;
  //MBC7
  if(name == "X"     ) mapping = virtualPads[0].lx;
  if(name == "Y"     ) mapping = virtualPads[0].ly;

  if(mapping) {
    auto value = mapping->value();
    if(auto axis = node->cast<ares::Node::Input::Axis>()) {
      axis->setValue(value);
    }
    if(auto button = node->cast<ares::Node::Input::Button>()) {
      button->setValue(value);
    }
    if(auto rumble = node->cast<ares::Node::Input::Rumble>()) {
      if(auto target = dynamic_cast<InputRumble*>(mapping.data())) {
        target->rumble(rumble->enable());
      }
    }
  }
}

GameBoyColor::GameBoyColor() {
  medium = mia::medium("Game Boy Color");
  manufacturer = "Nintendo";
  name = "Game Boy Color";
}

auto GameBoyColor::load() -> bool {
  if(!ares::GameBoy::load(root, "[Nintendo] Game Boy Color")) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto fastBoot = root->find<ares::Node::Setting::Boolean>("Fast Boot")) {
    fastBoot->setValue(settings.boot.fast);
  }

  return true;
}

auto GameBoyColor::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(name == "manifest.bml") return Emulator::manifest();

  if(name == "boot.cgb-0.rom") {
    return vfs::memory::open(Resource::GameBoyColor::BootCGB0, sizeof Resource::GameBoyColor::BootCGB0);
  }

  auto document = BML::unserialize(game.manifest);
  auto programROMSize = document["game/board/memory(content=Program,type=ROM)/size"].natural();
  auto saveRAMVolatile = (bool)document["game/board/memory(Content=Save,type=RAM)/volatile"];

  if(name == "program.rom") {
    return vfs::memory::open(game.image.data(), programROMSize);
  }

  if(name == "save.ram" && !saveRAMVolatile) {
    auto location = locate(game.location, ".sav", settings.paths.saves);
    if(auto result = vfs::disk::open(location, mode)) return result;
  }

  if(name == "save.eeprom") {
    auto location = locate(game.location, ".sav", settings.paths.saves);
    if(auto result = vfs::disk::open(location, mode)) return result;
  }

  if(name == "download.flash") {
    auto location = locate(game.location, ".flash", settings.paths.saves);
    if(auto result = vfs::disk::open(location, mode)) return result;
  }

  if(name == "time.rtc") {
    auto location = locate(game.location, ".rtc", settings.paths.saves);
    if(auto result = vfs::disk::open(location, mode)) return result;
  }

  return {};
}

auto GameBoyColor::input(ares::Node::Input::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "Up"    ) mapping = virtualPads[0].up;
  if(name == "Down"  ) mapping = virtualPads[0].down;
  if(name == "Left"  ) mapping = virtualPads[0].left;
  if(name == "Right" ) mapping = virtualPads[0].right;
  if(name == "B"     ) mapping = virtualPads[0].a;
  if(name == "A"     ) mapping = virtualPads[0].b;
  if(name == "Select") mapping = virtualPads[0].select;
  if(name == "Start" ) mapping = virtualPads[0].start;
  //MBC5
  if(name == "Rumble") mapping = virtualPads[0].rumble;
  //MBC7
  if(name == "X"     ) mapping = virtualPads[0].lx;
  if(name == "Y"     ) mapping = virtualPads[0].ly;

  if(mapping) {
    auto value = mapping->value();
    if(auto axis = node->cast<ares::Node::Input::Axis>()) {
      axis->setValue(value);
    }
    if(auto button = node->cast<ares::Node::Input::Button>()) {
      button->setValue(value);
    }
    if(auto rumble = node->cast<ares::Node::Input::Rumble>()) {
      if(auto target = dynamic_cast<InputRumble*>(mapping.data())) {
        target->rumble(rumble->enable());
      }
    }
  }
}
