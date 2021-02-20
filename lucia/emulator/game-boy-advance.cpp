namespace ares::GameBoyAdvance {
  auto load(Node::System& node, string name) -> bool;
}

struct GameBoyAdvance : Emulator {
  GameBoyAdvance();
  auto load(Menu) -> void override;
  auto load() -> bool override;
  auto open(ares::Node::Object, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> override;
  auto input(ares::Node::Input::Input) -> void override;
};

GameBoyAdvance::GameBoyAdvance() {
  medium = mia::medium("Game Boy Advance");
  manufacturer = "Nintendo";
  name = "Game Boy Advance";

  firmware.append({"BIOS", "World", "fd2547724b505f487e6dcb29ec2ecff3af35a841a77ab2e85fd87350abd36570"});
}

auto GameBoyAdvance::load(Menu menu) -> void {
  Menu orientationMenu{&menu};
  orientationMenu.setText("Orientation").setIcon(Icon::Device::Display);
  if(auto orientations = root->find<ares::Node::Setting::String>("PPU/Screen/Orientation")) {
    Group group;
    for(auto& orientation : orientations->readAllowedValues()) {
      MenuRadioItem item{&orientationMenu};
      item.setText(orientation);
      item.onActivate([=] {
        if(auto orientations = root->find<ares::Node::Setting::String>("PPU/Screen/Orientation")) {
          orientations->setValue(orientation);
        }
      });
      group.append(item);
    }
  }
}

auto GameBoyAdvance::load() -> bool {
  if(!ares::GameBoyAdvance::load(root, "[Nintendo] Game Boy Player")) return false;

  if(!file::exists(firmware[0].location)) {
    errorFirmwareRequired(firmware[0]);
    return false;
  }

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  return true;
}

auto GameBoyAdvance::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(name == "manifest.bml") return Emulator::manifest();

  if(name == "bios.rom") {
    return loadFirmware(firmware[0]);
  }

  auto document = BML::unserialize(game.manifest);
  auto programROMSize = document["game/board/memory(content=Program,type=ROM)/size"].natural();
  auto saveRAMVolatile = (bool)document["game/board/memory(Content=Save)/volatile"];

  if(name == "program.rom") {
    return vfs::memory::open(game.image.data(), programROMSize);
  }

  if(name == "save.ram" || name == "save.eeprom" || name == "save.flash") {
    if(saveRAMVolatile) return {};
    auto location = locate(game.location, ".sav", settings.paths.saves);
    if(auto result = vfs::disk::open(location, mode)) return result;
  }

  return {};
}

auto GameBoyAdvance::input(ares::Node::Input::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "Up"    ) mapping = virtualPads[0].up;
  if(name == "Down"  ) mapping = virtualPads[0].down;
  if(name == "Left"  ) mapping = virtualPads[0].left;
  if(name == "Right" ) mapping = virtualPads[0].right;
  if(name == "B"     ) mapping = virtualPads[0].a;
  if(name == "A"     ) mapping = virtualPads[0].b;
  if(name == "L"     ) mapping = virtualPads[0].l1;
  if(name == "R"     ) mapping = virtualPads[0].r1;
  if(name == "Select") mapping = virtualPads[0].select;
  if(name == "Start" ) mapping = virtualPads[0].start;
  //Game Boy Player
  if(name == "Rumble") mapping = virtualPads[0].rumble;

  if(mapping) {
    auto value = mapping->value();
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
