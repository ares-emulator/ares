struct NeoGeoPocket : Emulator {
  NeoGeoPocket();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
};

NeoGeoPocket::NeoGeoPocket() {
  medium = mia::medium("Neo Geo Pocket");
  manufacturer = "SNK";
  name = "Neo Geo Pocket";

  firmware.append({"BIOS", "World", "0293555b21c4fac516d25199df7809b26beeae150e1d4504a050db32264a6ad7"});
}

auto NeoGeoPocket::load() -> bool {
  if(!file::exists(firmware[0].location)) {
    errorFirmwareRequired(firmware[0]);
    return false;
  }
  system.pak->append("bios.rom", loadFirmware(firmware[0]));
  system.pak->append("cpu.ram", 12_KiB);
  system.pak->append("apu.ram", 4_KiB);
  Emulator::load(system, "cpu.ram");
  Emulator::load(system, "apu.ram");

  if(!ares::NeoGeoPocket::load(root, "[SNK] Neo Geo Pocket")) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto fastBoot = root->find<ares::Node::Setting::Boolean>("Fast Boot")) {
    fastBoot->setValue(settings.boot.fast);
  }

  return true;
}

auto NeoGeoPocket::save() -> bool {
  root->save();
  Emulator::save(system, "cpu.ram");
  Emulator::save(system, "apu.ram");
  medium->save(game.location, game.pak);
  return true;
}

auto NeoGeoPocket::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Neo Geo Pocket") return system.pak;
  if(node->name() == "Neo Geo Pocket Cartridge") return game.pak;
  return {};
}

auto NeoGeoPocket::input(ares::Node::Input::Input node) -> void {
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
