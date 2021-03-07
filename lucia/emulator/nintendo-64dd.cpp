//not functional yet

struct Nintendo64DD : Emulator {
  Nintendo64DD();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;

  shared_pointer<mia::Pak> bios;
};

Nintendo64DD::Nintendo64DD() {
  manufacturer = "Nintendo";
  name = "Nintendo 64DD";

  firmware.append({"BIOS", "Japan"});
}

auto Nintendo64DD::load() -> bool {
  game = mia::Medium::create("Nintendo 64DD");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  bios = mia::Medium::create("Nintendo 64");
  if(!bios->load(firmware[0].location)) return errorFirmware(firmware[0]), false;

  system = mia::System::create("Nintendo 64");
  if(!system->load()) return false;

  ares::Nintendo64::option("Quality", settings.video.quality);
  ares::Nintendo64::option("Supersampling", settings.video.supersampling);

  auto region = Emulator::region();
  if(!ares::Nintendo64::load(root, {"[Nintendo] Nintendo 64 (", region, ")"})) return false;

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

auto Nintendo64DD::save() -> bool {
  root->save();
  system->save(system->location);
  bios->save(bios->location);
  game->save(game->location);
  return true;
}

auto Nintendo64DD::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Nintendo 64") return system->pak;
  if(node->name() == "Nintendo 64 Cartridge") return bios->pak;
  if(node->name() == "Nintendo 64DD Disk") return game->pak;
  return {};
}

auto Nintendo64DD::input(ares::Node::Input::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "X-Axis" ) mapping = virtualPads[0].lx;
  if(name == "Y-Axis" ) mapping = virtualPads[0].ly;
  if(name == "Up"     ) mapping = virtualPads[0].up;
  if(name == "Down"   ) mapping = virtualPads[0].down;
  if(name == "Left"   ) mapping = virtualPads[0].left;
  if(name == "Right"  ) mapping = virtualPads[0].right;
  if(name == "B"      ) mapping = virtualPads[0].a;
  if(name == "A"      ) mapping = virtualPads[0].b;
  if(name == "C-Up"   ) mapping = virtualPads[0].ry;
  if(name == "C-Down" ) mapping = virtualPads[0].ry;
  if(name == "C-Left" ) mapping = virtualPads[0].rx;
  if(name == "C-Right") mapping = virtualPads[0].rx;
  if(name == "L"      ) mapping = virtualPads[0].l1;
  if(name == "R"      ) mapping = virtualPads[0].r1;
  if(name == "Z"      ) mapping = virtualPads[0].z;
  if(name == "Start"  ) mapping = virtualPads[0].start;

  if(mapping) {
    auto value = mapping->value();
    if(auto axis = node->cast<ares::Node::Input::Axis>()) {
      axis->setValue(value);
    }
    if(auto button = node->cast<ares::Node::Input::Button>()) {
      if(name == "C-Up"   || name == "C-Left" ) return button->setValue(value < -16384);
      if(name == "C-Down" || name == "C-Right") return button->setValue(value > +16384);
      button->setValue(value);
    }
  }
}
