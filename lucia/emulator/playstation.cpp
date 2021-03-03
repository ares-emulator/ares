struct PlayStation : Emulator {
  PlayStation();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;

  Pak memoryCard;
  u32 regionID = 0;
};

PlayStation::PlayStation() {
  medium = mia::medium("PlayStation");
  manufacturer = "Sony";
  name = "PlayStation";

  firmware.append({"BIOS", "US"});      //NTSC-U
  firmware.append({"BIOS", "Japan"});   //NTSC-J
  firmware.append({"BIOS", "Europe"});  //PAL
}

auto PlayStation::load() -> bool {
  auto region = Emulator::region();
  //if statements below are ordered by lowest to highest priority
  if(region == "PAL"   ) regionID = 2;
  if(region == "NTSC-J") regionID = 1;
  if(region == "NTSC-U") regionID = 0;

  if(!file::exists(firmware[regionID].location)) {
    errorFirmwareRequired(firmware[regionID]);
    return false;
  }
  system.pak->append("bios.rom", loadFirmware(firmware[regionID]));

  if(!ares::PlayStation::load(root, {"[Sony] PlayStation (", region, ")"})) return false;

  if(auto fastBoot = root->find<ares::Node::Setting::Boolean>("Fast Boot")) {
    fastBoot->setValue(settings.boot.fast);
  }

  if(auto port = root->find<ares::Node::Port>("PlayStation/Disc Tray")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Digital Gamepad");
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Memory Card Port 1")) {
    memoryCard.pak = shared_pointer{new vfs::directory};
    medium->load(game.location, memoryCard.pak, "save.card", ".card", 128_KiB);
    port->allocate("Memory Card");
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 2")) {
    port->allocate("Digital Gamepad");
    port->connect();
  }

  return true;
}

auto PlayStation::save() -> bool {
  root->save();
  if(memoryCard.pak) medium->save(game.location, memoryCard.pak, "save.card", ".card");
  medium->save(game.location, game.pak);
  return true;
}

auto PlayStation::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "PlayStation") return system.pak;
  if(node->name() == "PlayStation Disc") return game.pak;
  if(node->name() == "Memory Card") return memoryCard.pak;
  return {};
}

auto PlayStation::input(ares::Node::Input::Input node) -> void {
  auto parent = ares::Node::parent(node);
  if(!parent) return;

  auto port = ares::Node::parent(parent);
  if(!port) return;

  maybe<u32> index;
  if(port->name() == "Controller Port 1") index = 0;
  if(port->name() == "Controller Port 2") index = 1;
  if(!index) return;

  if(parent->name() == "Digital Gamepad") {
    auto name = node->name();
    maybe<InputMapping&> mapping;
    if(name == "Up"      ) mapping = virtualPads[*index].up;
    if(name == "Down"    ) mapping = virtualPads[*index].down;
    if(name == "Left"    ) mapping = virtualPads[*index].left;
    if(name == "Right"   ) mapping = virtualPads[*index].right;
    if(name == "Cross"   ) mapping = virtualPads[*index].a;
    if(name == "Circle"  ) mapping = virtualPads[*index].b;
    if(name == "Square"  ) mapping = virtualPads[*index].x;
    if(name == "Triangle") mapping = virtualPads[*index].y;
    if(name == "L1"      ) mapping = virtualPads[*index].l1;
    if(name == "L2"      ) mapping = virtualPads[*index].l2;
    if(name == "R1"      ) mapping = virtualPads[*index].r1;
    if(name == "R2"      ) mapping = virtualPads[*index].r2;
    if(name == "Select"  ) mapping = virtualPads[*index].select;
    if(name == "Start"   ) mapping = virtualPads[*index].start;

    if(mapping) {
      auto value = mapping->value();
      if(auto axis = node->cast<ares::Node::Input::Axis>()) {
        axis->setValue(value);
      }
      if(auto button = node->cast<ares::Node::Input::Button>()) {
        button->setValue(value);
      }
    }
  }
}
