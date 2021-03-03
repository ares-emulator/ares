struct NeoGeoPocketColor : Emulator {
  NeoGeoPocketColor();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
};

NeoGeoPocketColor::NeoGeoPocketColor() {
  medium = mia::medium("Neo Geo Pocket Color");
  manufacturer = "SNK";
  name = "Neo Geo Pocket Color";

  firmware.append({"BIOS", "World", "8fb845a2f71514cec20728e2f0fecfade69444f8d50898b92c2259f1ba63e10d"});
}

auto NeoGeoPocketColor::load() -> bool {
  if(!file::exists(firmware[0].location)) {
    errorFirmwareRequired(firmware[0]);
    return false;
  }
  system.pak->append("bios.rom", loadFirmware(firmware[0]));
  system.pak->append("cpu.ram", 12_KiB);
  system.pak->append("apu.ram", 4_KiB);
  Emulator::load(system, "cpu.ram");
  Emulator::load(system, "apu.ram");

  if(!ares::NeoGeoPocket::load(root, "[SNK] Neo Geo Pocket Color")) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto fastBoot = root->find<ares::Node::Setting::Boolean>("Fast Boot")) {
    fastBoot->setValue(settings.boot.fast);
  }

  return true;
}

auto NeoGeoPocketColor::save() -> bool {
  root->save();
  Emulator::save(system, "cpu.ram");
  Emulator::save(system, "apu.ram");
  medium->save(game.location, game.pak);
  return true;
}

auto NeoGeoPocketColor::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Neo Geo Pocket Color") return system.pak;
  if(node->name() == "Neo Geo Pocket Color Cartridge") return game.pak;
  return {};
}

auto NeoGeoPocketColor::input(ares::Node::Input::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "Up"    ) mapping = virtualPads[0].up;
  if(name == "Down"  ) mapping = virtualPads[0].down;
  if(name == "Left"  ) mapping = virtualPads[0].left;
  if(name == "Right" ) mapping = virtualPads[0].right;
  if(name == "A"     ) mapping = virtualPads[0].a;
  if(name == "B"     ) mapping = virtualPads[0].b;
  if(name == "Option") mapping = virtualPads[0].start;

  if(mapping) {
    auto value = mapping->value();
    if(auto button = node->cast<ares::Node::Input::Button>()) {
      button->setValue(value);
    }
  }
}
