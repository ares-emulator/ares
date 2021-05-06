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
  maybe<InputMapping&> mappings[2];
  if(name == "X-Axis" ) mappings[0] = virtualPads[0].lleft, mappings[1] = virtualPads[0].lright;
  if(name == "Y-Axis" ) mappings[0] = virtualPads[0].lup,   mappings[1] = virtualPads[1].ldown;
  if(name == "Up"     ) mappings[0] = virtualPads[0].up;
  if(name == "Down"   ) mappings[0] = virtualPads[0].down;
  if(name == "Left"   ) mappings[0] = virtualPads[0].left;
  if(name == "Right"  ) mappings[0] = virtualPads[0].right;
  if(name == "B"      ) mappings[0] = virtualPads[0].a;
  if(name == "A"      ) mappings[0] = virtualPads[0].b;
  if(name == "C-Up"   ) mappings[0] = virtualPads[0].rup;
  if(name == "C-Down" ) mappings[0] = virtualPads[0].rdown;
  if(name == "C-Left" ) mappings[0] = virtualPads[0].rleft;
  if(name == "C-Right") mappings[0] = virtualPads[0].rright;
  if(name == "L"      ) mappings[0] = virtualPads[0].l1;
  if(name == "R"      ) mappings[0] = virtualPads[0].r1;
  if(name == "Z"      ) mappings[0] = virtualPads[0].z;
  if(name == "Start"  ) mappings[0] = virtualPads[0].start;
  if(name == "Rumble" ) mappings[0] = virtualPads[0].rumble;

  if(mappings[0]) {
    if(auto axis = node->cast<ares::Node::Input::Axis>()) {
      auto value = mappings[1]->value() - mappings[0]->value();
      axis->setValue(value);
    }
    if(auto button = node->cast<ares::Node::Input::Button>()) {
      auto value = mappings[0]->value();
      if(name.beginsWith("C-")) value = abs(value) > +16384;
      button->setValue(value);
    }
    if(auto rumble = node->cast<ares::Node::Input::Rumble>()) {
      if(auto target = dynamic_cast<InputRumble*>(mappings[0].data())) {
        target->rumble(rumble->enable());
      }
    }
  }
}
